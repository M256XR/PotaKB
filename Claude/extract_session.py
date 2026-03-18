#!/usr/bin/env python3
"""
Claude Code セッション原文抽出スクリプト
~/.claude/projects/D--Projects-PotaKB/*.jsonl から会話を読み取り
Claude/chat_logs/原文/YYYY-MM-DD_SessionNN.txt として保存する

使い方:
    python Claude/extract_session.py              # 最新セッションを変換
    python Claude/extract_session.py <uuid>       # 指定セッションを変換
    python Claude/extract_session.py --list       # 変換可能なセッション一覧
"""

import json
import os
import sys
import re
from pathlib import Path
from datetime import datetime, timezone


_possible_dirs = [
    Path.home() / ".claude" / "projects" / "C--Users-user-Documents-project",
    Path.home() / ".claude" / "projects" / "-home-user-Projects-PotaKB",
    Path.home() / ".claude" / "projects" / "D--Projects-PotaKB",
    Path.home() / ".claude" / "projects" / "C--Users-yuu-Documents-PotaKB-PotaKB",
]
JSONL_DIR = next((d for d in _possible_dirs if d.exists()), _possible_dirs[0])
LOG_DIR   = Path(__file__).parent / "chat_logs" / "原文"


def load_jsonl(jsonl_path: Path) -> list[dict]:
    messages = []
    with open(jsonl_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                messages.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return messages


def extract_conversation(messages: list[dict]) -> list[tuple[str, str]]:
    """(role, text) のリストを返す。role は 'User' or 'Claude'"""
    result = []
    for obj in messages:
        t = obj.get("type")

        if t == "user":
            content = obj.get("message", {}).get("content", "")
            if isinstance(content, str) and content.strip():
                result.append(("User", content.strip()))
            elif isinstance(content, list):
                # text ブロックだけ拾う（tool_result は除外）
                parts = [b["text"] for b in content
                         if isinstance(b, dict) and b.get("type") == "text"]
                if parts:
                    result.append(("User", "\n".join(parts)))

        elif t == "assistant":
            content = obj.get("message", {}).get("content", [])
            if isinstance(content, list):
                parts = [b["text"] for b in content
                         if isinstance(b, dict) and b.get("type") == "text"]
                if parts:
                    result.append(("Claude", "\n".join(parts)))

    return result


def get_session_date(messages: list[dict]) -> str:
    """最初のメッセージのタイムスタンプから日付を取得"""
    for obj in messages:
        ts = obj.get("timestamp")
        if ts:
            dt = datetime.fromisoformat(ts.replace("Z", "+00:00"))
            local_dt = dt.astimezone()
            return local_dt.strftime("%Y-%m-%d")
    return datetime.now().strftime("%Y-%m-%d")


def next_session_number() -> int:
    """chat_logs/原文/ の既存ファイルから次のセッション番号を決める"""
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    nums = []
    for p in LOG_DIR.glob("*_Session*.txt"):
        m = re.search(r"Session(\d+)\.txt$", p.name)
        if m:
            nums.append(int(m.group(1)))
    return max(nums, default=0) + 1


def save_session(jsonl_path: Path, session_num: int | None = None) -> Path:
    messages = load_jsonl(jsonl_path)
    conversation = extract_conversation(messages)

    if not conversation:
        print(f"[skip] 会話が空: {jsonl_path.name}")
        return None

    date_str = get_session_date(messages)
    num = session_num if session_num is not None else next_session_number()
    out_path = LOG_DIR / f"{date_str}_Session{num:02d}.txt"

    with open(out_path, "w", encoding="utf-8") as f:
        f.write(f"# PotaKB Session{num:02d}  {date_str}\n")
        f.write(f"# Source: {jsonl_path.name}\n")
        f.write("=" * 60 + "\n\n")
        for role, text in conversation:
            f.write(f"[{role}]\n{text}\n\n")
            f.write("-" * 40 + "\n\n")

    print(f"[ok] 保存: {out_path}")
    return out_path


def list_sessions():
    """変換可能なセッション一覧を表示"""
    if not JSONL_DIR.exists():
        print(f"[error] JSONLディレクトリが見つかりません: {JSONL_DIR}")
        return
    jsonl_files = sorted(JSONL_DIR.glob("*.jsonl"), key=lambda p: p.stat().st_mtime)
    print(f"{'UUID':40}  {'更新日時':20}  {'行数':>6}")
    print("-" * 72)
    for p in jsonl_files:
        mtime = datetime.fromtimestamp(p.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S")
        with open(p, encoding="utf-8") as f:
            lines = sum(1 for _ in f)
        print(f"{p.stem:40}  {mtime:20}  {lines:6}")


def main():
    if len(sys.argv) > 1:
        arg = sys.argv[1]
        if arg == "--list":
            list_sessions()
            return
        # UUID 指定
        jsonl_path = JSONL_DIR / f"{arg}.jsonl"
        if not jsonl_path.exists():
            print(f"[error] 見つからない: {jsonl_path}")
            sys.exit(1)
        save_session(jsonl_path)
    else:
        # 最新のセッション（最終更新が新しいもの）
        if not JSONL_DIR.exists():
            print(f"[error] JSONLディレクトリが見つかりません: {JSONL_DIR}")
            sys.exit(1)
        jsonl_files = sorted(JSONL_DIR.glob("*.jsonl"), key=lambda p: p.stat().st_mtime)
        if not jsonl_files:
            print("[error] JSONL ファイルが見つかりません")
            sys.exit(1)
        latest = jsonl_files[-1]
        print(f"最新セッション: {latest.name}")
        save_session(latest)


if __name__ == "__main__":
    main()
