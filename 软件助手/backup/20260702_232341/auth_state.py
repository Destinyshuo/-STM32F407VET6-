import json
from pathlib import Path
from typing import Dict, Optional, Tuple


class AuthStateManager:
    def __init__(self, storage_path: Optional[Path] = None) -> None:
        base_dir = Path(__file__).resolve().parent
        self.storage_path = storage_path or base_dir / "data" / "auth_state.json"
        self.storage_path.parent.mkdir(parents=True, exist_ok=True)
        self._data = self._load()

    def _default_data(self) -> Dict:
        return {
            "accounts": {},
            "active_user": None,
            "session_logged_in": False,
        }

    def _load(self) -> Dict:
        if not self.storage_path.exists():
            data = self._default_data()
            self._save(data)
            return data
        try:
            return json.loads(self.storage_path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            data = self._default_data()
            self._save(data)
            return data

    def _save(self, data: Optional[Dict] = None) -> None:
        if data is not None:
            self._data = data
        self.storage_path.write_text(
            json.dumps(self._data, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )

    def has_logged_in_session(self) -> bool:
        return bool(self._data.get("session_logged_in") and self._data.get("active_user"))

    def get_active_user(self) -> Optional[str]:
        return self._data.get("active_user")

    def has_account(self, phone: str) -> bool:
        return phone in self._data.get("accounts", {})

    def register_user(self, phone: str, password: str) -> Tuple[bool, str]:
        phone = phone.strip()
        if not phone.isdigit() or len(phone) != 11:
            return False, "手机号需为 11 位数字。"
        if len(password) < 4:
            return False, "密码长度至少为 4 位。"
        accounts = self._data.setdefault("accounts", {})
        if phone in accounts:
            return False, "该手机号已注册，请直接登录。"
        accounts[phone] = {"password": password}
        self._data["active_user"] = phone
        self._data["session_logged_in"] = True
        self._save()
        return True, "注册成功。"

    def login_user(self, phone: str, password: str) -> Tuple[bool, str]:
        phone = phone.strip()
        account = self._data.get("accounts", {}).get(phone)
        if account is None:
            return False, "账号不存在，请先注册。"
        if account.get("password") != password:
            return False, "密码错误，请重新输入。"
        self._data["active_user"] = phone
        self._data["session_logged_in"] = True
        self._save()
        return True, "登录成功。"

    def logout_user(self) -> None:
        self._data["session_logged_in"] = False
        self._data["active_user"] = None
        self._save()
