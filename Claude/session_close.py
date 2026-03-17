#!/usr/bin/env python3
"""
session_close.py - PotaKB セッション終了処理（機械的作業）

使い方:
    python Claude/session_close.py "sessionNN終了処理・○○追加"

処理内容:
    1. PROGRESS.md の古いセッションログを archive/PROGRESS_history.md に移動
    2. git add -A + commit + push
"""

import re
import subprocess
import sys
from pathlib import Path

KEEP_SESSIONS = 4        # PROGRESS.md に残すセッション数
LOG_SECTION = "## 直近の決定事項ログ"


def archive_progress(repo_root: Path) -> None:
    progress_path = repo_root / "Claude" / "PROGRESS.md"
    archive_path = repo_root / "Claude" / "archive" / "PROGRESS_history.md"

    content = progress_path.read_text(encoding="utf-8")

    # セクション開始位置を特定
    section_idx = content.find(f"\n{LOG_SECTION}\n")
    if section_idx == -1:
        print(f"  '{LOG_SECTION}' セクションが見つかりません。スキップします。")
        return

    # セクション終了位置（次の ## ヘッダー）を特定
    rest = content[section_idx + 1:]
    next_section = re.search(r'\n## ', rest)
    section_end = section_idx + 1 + (next_section.start() if next_section else len(rest))

    before = content[:section_idx]              # セクション前
    section = content[section_idx:section_end]  # 対象セクション
    after = content[section_end:]               # セクション後

    # ### エントリに分割（先頭の \n### で分割）
    parts = re.split(r'(?=\n### )', section)
    section_header = parts[0]   # "## 直近の決定事項ログ\n\n"
    entries = parts[1:]         # 各セッションエントリ

    if len(entries) <= KEEP_SESSIONS:
        print(f"  セッション数が {KEEP_SESSIONS} 件以下のためアーカイブ不要です。")
        return

    recent = entries[:KEEP_SESSIONS]
    old = entries[KEEP_SESSIONS:]

    # archive に追記
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    old_text = "".join(old)
    if archive_path.exists():
        existing = archive_path.read_text(encoding="utf-8")
        archive_path.write_text(existing + old_text, encoding="utf-8")
    else:
        archive_path.write_text(f"# PROGRESS 決定事項ログ アーカイブ\n\n{old_text}", encoding="utf-8")

    # PROGRESS.md を更新
    new_section = section_header + "".join(recent)
    progress_path.write_text(before + new_section + after, encoding="utf-8")
    print(f"  {len(old)} セッション分を archive/PROGRESS_history.md に移動しました。")


def git_commit_push(repo_root: Path, message: str) -> None:
    def run(cmd: list[str]) -> str:
        result = subprocess.run(
            cmd, cwd=repo_root, capture_output=True,
            text=True, encoding="utf-8", errors="replace"
        )
        if result.returncode != 0:
            print(f"  エラー: {' '.join(cmd)}")
            print(result.stderr)
            sys.exit(1)
        return (result.stdout or "").strip()

    print("  git add -A ...")
    run(["git", "add", "-A"])

    print("  git commit ...")
    run(["git", "commit", "-m", message])

    print("  git push ...")
    run(["git", "push"])


def main() -> None:
    if len(sys.argv) < 2:
        print("使い方: python Claude/session_close.py \"コミットメッセージ\"")
        sys.exit(1)

    commit_message = sys.argv[1]
    repo_root = Path(__file__).resolve().parent.parent

    print("=== session_close.py ===")

    print("[1] PROGRESS.md アーカイブ中...")
    archive_progress(repo_root)

    print("[2] git add / commit / push ...")
    git_commit_push(repo_root, commit_message)

    print("=== 完了 ===")


if __name__ == "__main__":
    main()
