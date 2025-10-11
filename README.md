# PotaKBとは 
70x130mmほど65キーの小型キーボードです  
なるべく薄く作ることを目的としています  
  
# ファイルについて  
old\_fw内のファームウェアはbluetooth経由でキーマップ変更ができません  
program/PotaKB\_fw/PotaKB\_Configurator.html keymap.hの作成、bluetoothでのキーマップ変更   
program/PotaKB\_fw/PotaKB\_fw.ino            ファームウェア  
program/PotaKB\_fw/keymap.h                 キーマップ  
PotaKB\_fw.inoとkeymap.hを同じ階層に配置して書き込みしてください  
lib  
Adafruit Bluefruit nRF52 Library  
Adafruit MCP23X17 Library  
Adafruit TinyUSB Library  
  
これらプログラムのAIを使用して作られています  
  
作者はプログラムが書けな過ぎて  
zmk,qmk,kmkでの対応ができませんでした  
