from maix import camera, display, image, nn, app, time
import os
import struct
import serial
import cv2

# ------------------------- config -------------------------
SERIAL_PORT = "/dev/ttyS0"
SERIAL_BAUD = 115200
SERIAL_TIMEOUT = 0
SERIAL_WRITE_TIMEOUT = 0.02
SERIAL_DEBUG = False

LIGHT_PACKET_HEADER = b"\xA3\xB1"
STATE_PACKET_HEADER = b"\xA3\xB2"
PLANT_PACKET_HEADER = b"\xA3\xB3"
TX_PACKET_TAIL = b"\xC3"

RX_PACKET_HEADER = b"\xA4\xB4"
RX_PACKET_TAIL = b"\xC4"
RX_BUFFER_MAX = 64
RX_MODE_BUFFER = bytearray()
RX_MODE_QUEUE = []

MODE_PLANT = 0x01
MODE_LIGHT = 0x02
DEFAULT_MODE = MODE_PLANT

PLANT_SHADE = 0x04
PLANT_HALF_SUN = 0x05
PLANT_FULL_SUN = 0x06

STATE_NORMAL = 0
STATE_YELLOW = 1
STATE_ROT = 2
STATE_NONE = 3

CONF_TH = 0.50
IOU_TH = 0.45
STABLE_FRAMES = 3
SERIAL_RESEND_MS = 200

MIN_SCORE_NORMAL = 0.65
MIN_SCORE_ABNORMAL = 0.60
MIN_BOX_AREA_RATIO = 0.03
VOTE_WINDOW = 5

LIGHT_THRESH = 210
LIGHT_MIN_AREA_RATIO = 0.08
LIGHT_SEND_MS = 100
MODE_POLL_PACKETS_PER_CALL = 8

MODEL_CANDIDATES = [
    "model_288895.mud",
    "/root/models/model_288895.mud",
    "/root/models/model-288895.maixcam/model_288895.mud",
    "/root/models/maixhub/288895/model_288895.mud",
]

STATE_TEXT = {
    STATE_NORMAL: "NORMAL",
    STATE_YELLOW: "YELLOW",
    STATE_ROT: "ROT",
    STATE_NONE: "NO_LEAF",
}

PLANT_TEXT = {
    PLANT_SHADE: "Shade",
    PLANT_HALF_SUN: "Half Sun",
    PLANT_FULL_SUN: "Full Sun",
}

MODE_TEXT = {
    MODE_LIGHT: "LIGHT",
    MODE_PLANT: "PLANT",
}

STATE_COLOR = {
    STATE_NORMAL: image.COLOR_GREEN,
    STATE_YELLOW: image.COLOR_YELLOW,
    STATE_ROT: image.COLOR_RED,
    STATE_NONE: image.COLOR_WHITE,
}

STATE_SEVERITY = {
    STATE_NORMAL: 0,
    STATE_YELLOW: 1,
    STATE_ROT: 2,
    STATE_NONE: -1,
}

PLANT_BUTTON_COLOR = {
    PLANT_SHADE: image.Color(50, 100, 160),
    PLANT_HALF_SUN: image.Color(170, 120, 40),
    PLANT_FULL_SUN: image.Color(180, 80, 40),
}


def resolve_model_path():
    for p in MODEL_CANDIDATES:
        if os.path.exists(p):
            print("Using model:", p)
            return p
    raise FileNotFoundError("model_286780.mud not found in known paths")


def open_serial():
    try:
        ser = serial.Serial(
            SERIAL_PORT,
            SERIAL_BAUD,
            timeout=SERIAL_TIMEOUT,
            write_timeout=SERIAL_WRITE_TIMEOUT,
        )
        print("Serial opened:", SERIAL_PORT)
        return ser
    except Exception as e:
        print("Serial open failed:", e)
        return None


def open_touch():
    try:
        from maix import touchscreen  # type: ignore
        ts = touchscreen.TouchScreen()
        print("Touch opened")
        return ts
    except Exception as e:
        print("Touch open failed:", e)
        return None


def get_state_name(state_id):
    return STATE_TEXT.get(state_id, "UNKNOWN")


def get_state_color(state_id):
    return STATE_COLOR.get(state_id, image.COLOR_WHITE)


def get_plant_name(code):
    return PLANT_TEXT.get(code, "UNKNOWN")


def get_mode_name(code):
    return MODE_TEXT.get(code, "WAIT")


def draw_top_bar(img, text, color=image.COLOR_WHITE):
    img.draw_rect(0, 0, img.width(), 32, color=image.Color(0, 0, 0), thickness=-1)
    img.draw_string(8, 6, text, color=color, scale=1.2)


def draw_fps(img, fps):
    img.draw_string(8, 36, f"FPS: {fps:.1f}", color=image.COLOR_GREEN, scale=1.0)


def serial_debug(*args):
    if SERIAL_DEBUG:
        print(*args)


def send_packet(ser, header, payload=b"", tail=None, tag="UART"):
    if not ser or not ser.is_open:
        return False
    try:
        packet = header + payload
        if tail is not None:
            packet += tail
        ser.write(packet)
        serial_debug(f"{tag} TX:", packet.hex(" "))
        return True
    except Exception as e:
        print("Serial write failed:", e)
        return False


def send_state_packet(ser, state_id):
    payload = struct.pack("B", int(state_id) & 0xFF)
    return send_packet(ser, STATE_PACKET_HEADER, payload, TX_PACKET_TAIL, "STATE")


def send_plant_type_packet(ser, plant_code):
    payload = struct.pack("B", int(plant_code) & 0xFF)
    return send_packet(ser, PLANT_PACKET_HEADER, payload, TX_PACKET_TAIL, "PLANT")


def send_light_center_packet(ser, x, y):
    x = max(0, min(65535, int(x)))
    y = max(0, min(65535, int(y)))
    payload = struct.pack(">HH", x, y)
    return send_packet(ser, LIGHT_PACKET_HEADER, payload, TX_PACKET_TAIL, "LIGHT")


def send_light_not_found_packet(ser):
    return send_packet(ser, b"\xA3\xB4", b"", TX_PACKET_TAIL, "LIGHT_NONE")


def read_serial_available(ser):
    try:
        waiting = ser.in_waiting
    except Exception:
        waiting = 0

    if waiting > 0:
        return ser.read(waiting)

    data = ser.read(1)
    return data or b""


def _parse_mode_buffer():
    global RX_MODE_BUFFER

    while True:
        start = RX_MODE_BUFFER.find(RX_PACKET_HEADER)
        if start < 0:
            if len(RX_MODE_BUFFER) > 1:
                RX_MODE_BUFFER = RX_MODE_BUFFER[-1:]
            break

        if start > 0:
            RX_MODE_BUFFER = RX_MODE_BUFFER[start:]

        end = RX_MODE_BUFFER.find(RX_PACKET_TAIL, len(RX_PACKET_HEADER))
        if end < 0:
            break

        packet = bytes(RX_MODE_BUFFER[: end + 1])
        del RX_MODE_BUFFER[: end + 1]
        serial_debug("UART RX:", packet.hex(" "))

        payload = packet[len(RX_PACKET_HEADER):-1]
        if len(payload) == 1 and payload[0] in (MODE_PLANT, MODE_LIGHT):
            RX_MODE_QUEUE.append(payload[0])


def recv_mode_packet(ser):
    if not ser or not ser.is_open:
        return None

    try:
        global RX_MODE_BUFFER

        if RX_MODE_QUEUE:
            return RX_MODE_QUEUE.pop(0)

        data = read_serial_available(ser)
        if data:
            RX_MODE_BUFFER.extend(data)

        if len(RX_MODE_BUFFER) > RX_BUFFER_MAX:
            RX_MODE_BUFFER = RX_MODE_BUFFER[-RX_BUFFER_MAX:]

        _parse_mode_buffer()

        if RX_MODE_QUEUE:
            return RX_MODE_QUEUE.pop(0)
        return None
    except Exception as e:
        print("Serial read failed:", e)
        return None

    return None


def recv_mode_switch(ser, current_mode):
    checked = 0
    while checked < MODE_POLL_PACKETS_PER_CALL:
        recv_code = recv_mode_packet(ser)
        if recv_code not in (MODE_LIGHT, MODE_PLANT):
            return None
        if recv_code != current_mode:
            return recv_code
        checked += 1
    return None


def read_touch_point(ts):
    if ts is None:
        return None

    try:
        point = ts.read()
    except Exception:
        return None

    if point is None:
        return None

    if isinstance(point, (tuple, list)) and len(point) >= 3:
        return int(point[0]), int(point[1]), bool(point[2])

    x = getattr(point, "x", None)
    y = getattr(point, "y", None)
    pressed = getattr(point, "pressed", True)
    if x is None or y is None:
        return None
    return int(x), int(y), bool(pressed)


def map_touch_to_image(ts, img, dis):
    p = read_touch_point(ts)
    if p is None:
        return None

    x, y, pressed = p
    try:
        img_x, img_y = image.resize_map_pos_reverse(
            img.width(),
            img.height(),
            dis.width(),
            dis.height(),
            image.Fit.FIT_CONTAIN,
            x,
            y,
        )
        if img_x < 0:
            img_x = 0
        if img_y < 0:
            img_y = 0
        return int(img_x), int(img_y), pressed
    except Exception:
        return int(x), int(y), pressed


def point_in_rect(x, y, rect):
    rx, ry, rw, rh = rect
    return rx <= x <= rx + rw and ry <= y <= ry + rh


def get_plant_button_rects(img_w=320):
    btn_w = img_w - 56
    btn_h = 36
    x = (img_w - btn_w) // 2
    return {
        PLANT_SHADE: (x, 68, btn_w, btn_h),
        PLANT_HALF_SUN: (x, 118, btn_w, btn_h),
        PLANT_FULL_SUN: (x, 168, btn_w, btn_h),
    }


def draw_plant_select_screen(img, selected_code=None):
    img.draw_rect(0, 0, img.width(), img.height(), color=image.Color(0, 0, 0), thickness=-1)

    title = "Select Plant Type"
    title_size = image.string_size(title, 1.2)
    img.draw_string((img.width() - title_size.width()) // 2, 18, title, color=image.COLOR_WHITE, scale=1.2)

    button_rects = get_plant_button_rects(img.width())
    options = [
        (PLANT_SHADE, "Shade"),
        (PLANT_HALF_SUN, "Half Sun"),
        (PLANT_FULL_SUN, "Full Sun"),
    ]

    for code, text in options:
        x, y, w, h = button_rects[code]
        bg = PLANT_BUTTON_COLOR[code] if code == selected_code else image.Color(48, 48, 48)
        border = image.COLOR_YELLOW if code == selected_code else image.Color(90, 90, 90)
        img.draw_rect(x, y, w, h, color=bg, thickness=-1)
        img.draw_rect(x, y, w, h, color=border, thickness=2)
        size = image.string_size(text, 1.2)
        tx = x + (w - size.width()) // 2
        ty = y + (h - size.height()) // 2
        img.draw_string(tx, ty, text, color=image.COLOR_WHITE, scale=1.2)


def draw_wait_mode_screen(img, plant_code, current_mode):
    img.draw_rect(0, 0, img.width(), img.height(), color=image.Color(0, 0, 0), thickness=-1)
    title = f"Plant: {get_plant_name(plant_code)}"
    size = image.string_size(title, 1.2)
    img.draw_string((img.width() - size.width()) // 2, 70, title, color=image.COLOR_YELLOW, scale=1.2)

    msg1 = f"Mode: {get_mode_name(current_mode)}"
    msg2 = "MCU: 01 Plant 02 Light"
    s1 = image.string_size(msg1, 1.1)
    s2 = image.string_size(msg2, 1.0)
    img.draw_string((img.width() - s1.width()) // 2, 115, msg1, color=image.COLOR_WHITE, scale=1.1)
    img.draw_string((img.width() - s2.width()) // 2, 145, msg2, color=image.COLOR_GREEN, scale=1.0)


def summarize_objects(objs):
    if not objs:
        return None, None

    def sort_key(obj):
        sev = STATE_SEVERITY.get(int(obj.class_id), 0)
        return (sev, float(obj.score))

    best_obj = max(objs, key=sort_key)
    return best_obj, int(best_obj.class_id)


def is_valid_detection(obj, img_w, img_h):
    area = float(obj.w * obj.h)
    area_ratio = area / float(img_w * img_h)
    class_id = int(obj.class_id)
    score = float(obj.score)

    if area_ratio < MIN_BOX_AREA_RATIO:
        return False

    if class_id == STATE_NORMAL:
        return score >= MIN_SCORE_NORMAL
    return score >= MIN_SCORE_ABNORMAL


def filter_objects(objs, img_w, img_h):
    out = []
    for obj in objs:
        if is_valid_detection(obj, img_w, img_h):
            out.append(obj)
    return out


def vote_state(history):
    if not history:
        return STATE_NONE

    score_map = {
        STATE_NORMAL: 0.0,
        STATE_YELLOW: 0.0,
        STATE_ROT: 0.0,
        STATE_NONE: 0.0,
    }

    for idx, state in enumerate(history):
        score_map[state] += idx + 1

    if score_map[STATE_ROT] > 0:
        score_map[STATE_ROT] += 1.5
    elif score_map[STATE_YELLOW] > 0:
        score_map[STATE_YELLOW] += 1.0

    return max(score_map, key=lambda k: score_map[k])


def update_stable_state(current_state, pending_state, pending_count, stable_state):
    if current_state == pending_state:
        pending_count += 1
    else:
        pending_state = current_state
        pending_count = 1

    if pending_count >= STABLE_FRAMES:
        stable_state = pending_state

    return pending_state, pending_count, stable_state


def find_light_center(img):
    try:
        gray = img.to_format(image.Format.FMT_GRAYSCALE)
        gray_cv = image.image2cv(gray, False, False)
        _, binary = cv2.threshold(gray_cv, LIGHT_THRESH, 255, cv2.THRESH_BINARY)
        binary = cv2.GaussianBlur(binary, (5, 5), 0)
        _, binary = cv2.threshold(binary, LIGHT_THRESH, 255, cv2.THRESH_BINARY)

        contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        if not contours:
            return None

        img_area = img.width() * img.height()
        best = None
        best_area = 0

        for cnt in contours:
            area = cv2.contourArea(cnt)
            if area <= best_area:
                continue
            x, y, w, h = cv2.boundingRect(cnt)
            area_ratio = area / float(img_area)
            if area_ratio < LIGHT_MIN_AREA_RATIO:
                continue
            best_area = area
            best = (x, y, w, h)

        if best is None:
            return None

        x, y, w, h = best
        cx = x + w // 2
        cy = y + h // 2
        return cx, cy, best_area, (x, y, w, h)
    except Exception as e:
        print("Light detect failed:", e)
        return None


def draw_light_preview_overlay(img, cx, cy, rect):
    x, y, w, h = rect
    img.draw_rect(x, y, w, h, color=image.COLOR_YELLOW, thickness=2)
    img.draw_circle(cx, cy, 5, color=image.COLOR_RED, thickness=-1)


def show_light_realtime_preview(dis, img):
    # display.show() updates the LCD preview and sends the frame to MaixVision.
    dis.show(img)


def select_plant_type(ser, dis, cam, ts):
    selected_code = None
    selected_tick = 0
    last_touch_ms = 0
    touch_debounce_ms = 300
    rects = get_plant_button_rects(cam.width())

    while not app.need_exit():
        img = cam.read()

        tp = map_touch_to_image(ts, img, dis)
        if tp is not None:
            tx, ty, pressed = tp
            now_ms = time.ticks_ms()
            if pressed and now_ms - last_touch_ms > touch_debounce_ms:
                for code, rect in rects.items():
                    if point_in_rect(tx, ty, rect):
                        selected_code = code
                        selected_tick = now_ms
                        last_touch_ms = now_ms
                        break

        draw_plant_select_screen(img, selected_code)
        dis.show(img)

        if selected_code is not None and time.ticks_ms() - selected_tick > 800:
            return selected_code

    return None


def wait_mode_command(ser, dis, cam, plant_code, current_mode):
    while not app.need_exit():
        recv_code = recv_mode_packet(ser)
        if recv_code in (MODE_LIGHT, MODE_PLANT):
            return recv_code

        img = cam.read()
        draw_wait_mode_screen(img, plant_code, current_mode)
        dis.show(img)

        recv_code = recv_mode_packet(ser)
        if recv_code in (MODE_LIGHT, MODE_PLANT):
            return recv_code

    return None


def run_light_mode(ser, cam, dis, plant_code):
    fps = 0.0
    tick = time.ticks_ms()
    last_light_send_ms = 0
    last_light_none_send_ms = 0

    while not app.need_exit():
        recv_code = recv_mode_switch(ser, MODE_LIGHT)
        if recv_code is not None:
            return recv_code

        img = cam.read()
        recv_code = recv_mode_switch(ser, MODE_LIGHT)
        if recv_code is not None:
            return recv_code

        result = find_light_center(img)
        recv_code = recv_mode_switch(ser, MODE_LIGHT)
        if recv_code is not None:
            return recv_code

        if result is not None:
            cx, cy, area, rect = result
            draw_light_preview_overlay(img, cx, cy, rect)

            now_ms = time.ticks_ms()
            if now_ms - last_light_send_ms >= LIGHT_SEND_MS:
                send_light_center_packet(ser, cx, cy)
                last_light_send_ms = now_ms
        else:
            now_ms = time.ticks_ms()
            if now_ms - last_light_none_send_ms >= LIGHT_SEND_MS:
                send_light_not_found_packet(ser)
                last_light_none_send_ms = now_ms

        draw_top_bar(img, "LIGHT MODE", image.COLOR_YELLOW)

        new_tick = time.ticks_ms()
        dt = new_tick - tick
        if dt > 0:
            fps = 1000.0 / dt
        tick = new_tick
        draw_fps(img, fps)

        show_light_realtime_preview(dis, img)

        recv_code = recv_mode_switch(ser, MODE_LIGHT)
        if recv_code is not None:
            return recv_code


def run_plant_mode(ser, cam, dis, detector, plant_code):
    tick = time.ticks_ms()
    fps = 0.0
    pending_state = None
    pending_count = 0
    stable_state = STATE_NONE
    last_sent_state = None
    last_send_ms = 0
    state_history = []

    while not app.need_exit():
        recv_code = recv_mode_switch(ser, MODE_PLANT)
        if recv_code is not None:
            return recv_code

        img = cam.read()
        recv_code = recv_mode_switch(ser, MODE_PLANT)
        if recv_code is not None:
            return recv_code

        objs = detector.detect(img, conf_th=CONF_TH, iou_th=IOU_TH)
        recv_code = recv_mode_switch(ser, MODE_PLANT)
        if recv_code is not None:
            return recv_code

        objs = filter_objects(objs, img.width(), img.height())

        best_obj, current_state = summarize_objects(objs)
        if current_state is None:
            current_state = STATE_NONE

        state_history.append(current_state)
        if len(state_history) > VOTE_WINDOW:
            state_history.pop(0)

        current_state = vote_state(state_history)
        pending_state, pending_count, stable_state = update_stable_state(
            current_state, pending_state, pending_count, stable_state
        )

        for obj in objs:
            cid = int(obj.class_id)
            color = get_state_color(cid)
            img.draw_rect(obj.x, obj.y, obj.w, obj.h, color=color, thickness=2)

        if best_obj is not None:
            cx = int(best_obj.x + best_obj.w / 2)
            cy = int(best_obj.y + best_obj.h / 2)
            img.draw_circle(cx, cy, 4, color=get_state_color(int(best_obj.class_id)), thickness=-1)

        draw_top_bar(img, get_state_name(stable_state), get_state_color(stable_state))

        new_tick = time.ticks_ms()
        dt = new_tick - tick
        if dt > 0:
            fps = 1000.0 / dt
        tick = new_tick
        draw_fps(img, fps)

        now_ms = time.ticks_ms()
        if stable_state != last_sent_state or now_ms - last_send_ms >= SERIAL_RESEND_MS:
            if send_state_packet(ser, stable_state):
                last_sent_state = stable_state
                last_send_ms = now_ms

        dis.show(img)

        recv_code = recv_mode_switch(ser, MODE_PLANT)
        if recv_code is not None:
            return recv_code


def main():
    model_path = resolve_model_path()
    detector = nn.YOLOv5(model=model_path)

    cam = camera.Camera(detector.input_width(), detector.input_height(), detector.input_format())
    dis = display.Display()
    ser = open_serial()
    ts = open_touch()

    plant_code = select_plant_type(ser, dis, cam, ts)
    if plant_code is None:
        return
    print("Plant selected:", get_plant_name(plant_code), plant_code)
    send_plant_type_packet(ser, plant_code)

    mode = DEFAULT_MODE
    print("Default mode:", get_mode_name(mode), mode)

    while not app.need_exit():
        if mode == MODE_LIGHT:
            next_mode = run_light_mode(ser, cam, dis, plant_code)
            if next_mode in (MODE_LIGHT, MODE_PLANT):
                mode = next_mode
            else:
                mode = DEFAULT_MODE
        elif mode == MODE_PLANT:
            next_mode = run_plant_mode(ser, cam, dis, detector, plant_code)
            if next_mode in (MODE_LIGHT, MODE_PLANT):
                mode = next_mode
            else:
                mode = DEFAULT_MODE
        else:
            mode = wait_mode_command(ser, dis, cam, plant_code, DEFAULT_MODE)
            if mode is None:
                mode = DEFAULT_MODE


if __name__ == "__main__":
    main()
