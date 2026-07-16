#!/usr/bin/env python3
"""
BeatSync BPM 标注工具
分析音乐文件夹中的 MP3 文件 BPM，生成 bpm_list.csv

用法: python bpm_tagger.py /path/to/music/

依赖: pip install librosa soundfile
"""

import os
import sys
import csv

def analyze_bpm(filepath):
    """分析单个 MP3 文件的 BPM"""
    try:
        import librosa
        y, sr = librosa.load(filepath, sr=22050, mono=True)
        tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
        if hasattr(tempo, '__len__'):
            tempo = tempo[0]
        return round(float(tempo))
    except ImportError:
        print("错误: 请先安装 librosa (pip install librosa soundfile)")
        sys.exit(1)
    except Exception as e:
        print(f"  警告: 无法分析 {filepath}: {e}")
        return None

def main():
    if len(sys.argv) < 2:
        print("用法: python bpm_tagger.py <音乐文件夹路径>")
        print("示例: python bpm_tagger.py ./music/")
        sys.exit(1)

    music_dir = sys.argv[1]
    if not os.path.isdir(music_dir):
        print(f"错误: 目录不存在: {music_dir}")
        sys.exit(1)

    # 收集 MP3 文件
    mp3_files = sorted([
        f for f in os.listdir(music_dir)
        if f.lower().endswith('.mp3')
    ])

    if not mp3_files:
        print(f"未找到 MP3 文件: {music_dir}")
        sys.exit(1)

    print(f"找到 {len(mp3_files)} 个 MP3 文件，开始分析 BPM...\n")

    results = []
    for i, filename in enumerate(mp3_files, 1):
        filepath = os.path.join(music_dir, filename)
        print(f"[{i}/{len(mp3_files)}] {filename} ... ", end="", flush=True)
        bpm = analyze_bpm(filepath)
        if bpm:
            print(f"BPM={bpm}")
            results.append((i, filename, bpm))
        else:
            print("失败")

    # 写入 CSV
    csv_path = os.path.join(music_dir, "bpm_list.csv")
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["track_num", "filename", "bpm"])
        for track_num, filename, bpm in results:
            writer.writerow([track_num, filename, bpm])

    print(f"\n完成! bpm_list.csv 已保存到: {csv_path}")
    print(f"共标注 {len(results)} 首歌曲")

    # 打印汇总
    print("\nBPM 分布:")
    bpm_ranges = {"120-129": 0, "130-139": 0, "140-149": 0, "150-159": 0, "160-169": 0, "170+": 0}
    for _, _, bpm in results:
        if bpm < 130: bpm_ranges["120-129"] += 1
        elif bpm < 140: bpm_ranges["130-139"] += 1
        elif bpm < 150: bpm_ranges["140-149"] += 1
        elif bpm < 160: bpm_ranges["150-159"] += 1
        elif bpm < 170: bpm_ranges["160-169"] += 1
        else: bpm_ranges["170+"] += 1

    for r, count in bpm_ranges.items():
        print(f"  {r}: {count} 首")

if __name__ == "__main__":
    main()
