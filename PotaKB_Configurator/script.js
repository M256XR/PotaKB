document.addEventListener('DOMContentLoaded', () => {
    const KEYCODES = { "Special": ["KC_NO", "KC_TRNS", "L_LOWER", "SW_USB_BT", "KC_RESET_KM", "KC_REBOOT_DEF"], "Alphabet": [...Array(26)].map((_, i) => `HID_KEY_${String.fromCharCode(65 + i)}`), "Numbers": [...Array(10)].map((_, i) => `HID_KEY_${(i + 1) % 10}`), "Function": [...Array(12)].map((_, i) => `HID_KEY_F${i + 1}`), "Modifiers": ["HID_KEY_CONTROL_LEFT", "HID_KEY_SHIFT_LEFT", "HID_KEY_ALT_LEFT", "HID_KEY_GUI_LEFT", "HID_KEY_CONTROL_RIGHT", "HID_KEY_SHIFT_RIGHT", "HID_KEY_ALT_RIGHT", "HID_KEY_GUI_RIGHT"], "Control & Whitespace": ["HID_KEY_ENTER", "HID_KEY_ESCAPE", "HID_KEY_BACKSPACE", "HID_KEY_DELETE", "HID_KEY_TAB", "HID_KEY_SPACE", "HID_KEY_APPLICATION", "HID_KEY_CAPS_LOCK"], "Navigation": ["HID_KEY_ARROW_RIGHT", "HID_KEY_ARROW_LEFT", "HID_KEY_ARROW_DOWN", "HID_KEY_ARROW_UP", "HID_KEY_INSERT", "HID_KEY_HOME", "HID_KEY_END", "HID_KEY_PAGE_UP", "HID_KEY_PAGE_DOWN", "HID_KEY_PRINT_SCREEN", "HID_KEY_SCROLL_LOCK", "HID_KEY_PAUSE"], "Symbols (US)": ["HID_KEY_MINUS", "HID_KEY_EQUAL", "HID_KEY_BRACKET_LEFT", "HID_KEY_BRACKET_RIGHT", "HID_KEY_BACKSLASH", "HID_KEY_SEMICOLON", "HID_KEY_APOSTROPHE", "HID_KEY_GRAVE", "HID_KEY_COMMA", "HID_KEY_PERIOD", "HID_KEY_SLASH"], "JP Layout": ["HID_KEY_LANG1", "HID_KEY_LANG2", "HID_KEY_INTERNATIONAL1", "HID_KEY_INTERNATIONAL3"], "Mouse": ["KC_MS_BTN1", "KC_MS_BTN2", "KC_MS_BTN3", "KC_MS_BTN4", "KC_MS_BTN5", "KC_MS_UP", "KC_MS_DOWN", "KC_MS_LEFT", "KC_MS_RIGHT"] };
    const CATEGORY_DESCRIPTIONS = { "Special": "レイヤー切り替えやモード変更、プロファイルリセットなど、キーボードの特殊機能を制御します。", "Alphabet": "AからZまでのアルファベットキーです。", "Numbers": "0から9までの数字キーです。", "Function": "F1からF12までのファンクションキーです。", "Modifiers": "Ctrl、Shift、Alt、GUI (Windows/Command) などの修飾キーです。", "Control & Whitespace": "Enter、Esc、スペースキーなど、入力制御や空白に関連するキーです。", "Navigation": "矢印キーやHome、Endなど、カーソル移動やページ操作を行うキーです。", "Symbols (US)": "US配列における一般的な記号キーです。 (, ; [ ]など)", "JP Layout": "日本語配列特有のキーです。(半角/全角、無変換、変換、カタカナひらがな等)", "Mouse": "マウスのクリックやカーソル移動をキーに割り当てます。" };
    const KEYCODE_DESCRIPTIONS = { "KC_NO": "No Action: このキーを押しても何も起こりません。", "KC_TRNS": "Transparent: 下のレイヤーのキーを透過させます。 (レイヤー1以上で有効)", "L_LOWER": "Lower Layer: 押している間、レイヤー1 (Lower) に切り替えます。", "SW_USB_BT": "Mode Switch: USB接続とBluetooth接続を切り替えます。", "KC_RESET_KM": "Reset Keymap: 保存されたカスタムキーマップを完全に消去し、初期状態で再起動します。", "KC_REBOOT_DEF": "Reboot to Default: カスタムキーマップを維持したまま、一時的に初期設定で再起動します。", "HID_KEY_LANG1": "半角/全角・漢字キー (JP)", "HID_KEY_LANG2": "カタカナ・ひらがな・ローマ字キー (JP)", "HID_KEY_INTERNATIONAL1": "¥キー (JP)", "HID_KEY_INTERNATIONAL3": "_ (アンダースコア) キー (JP)" };
    const JIS_KEY_MAP = { 'HID_KEY_GRAVE': '半/全', 'HID_KEY_MINUS': 'ー', 'HID_KEY_EQUAL': '^', 'HID_KEY_BRACKET_LEFT': '@', 'HID_KEY_BRACKET_RIGHT': '[', 'HID_KEY_BACKSLASH': ']', 'HID_KEY_SEMICOLON': ';', 'HID_KEY_APOSTROPHE': ':', 'HID_KEY_INTERNATIONAL1': '¥', 'HID_KEY_INTERNATIONAL3': '\\' };
    const EVENT_CODE_TO_KEYCODE_MAP = { 'KeyA':'HID_KEY_A','KeyB':'HID_KEY_B','KeyC':'HID_KEY_C','KeyD':'HID_KEY_D','KeyE':'HID_KEY_E','KeyF':'HID_KEY_F','KeyG':'HID_KEY_G','KeyH':'HID_KEY_H','KeyI':'HID_KEY_I','KeyJ':'HID_KEY_J','KeyK':'HID_KEY_K','KeyL':'HID_KEY_L','KeyM':'HID_KEY_M','KeyN':'HID_KEY_N','KeyO':'HID_KEY_O','KeyP':'HID_KEY_P','KeyQ':'HID_KEY_Q','KeyR':'HID_KEY_R','KeyS':'HID_KEY_S','KeyT':'HID_KEY_T','KeyU':'HID_KEY_U','KeyV':'HID_KEY_V','KeyW':'HID_KEY_W','KeyX':'HID_KEY_X','KeyY':'HID_KEY_Y','KeyZ':'HID_KEY_Z', 'Digit1':'HID_KEY_1','Digit2':'HID_KEY_2','Digit3':'HID_KEY_3','Digit4':'HID_KEY_4','Digit5':'HID_KEY_5','Digit6':'HID_KEY_6','Digit7':'HID_KEY_7','Digit8':'HID_KEY_8','Digit9':'HID_KEY_9','Digit0':'HID_KEY_0', 'F1':'HID_KEY_F1','F2':'HID_KEY_F2','F3':'HID_KEY_F3','F4':'HID_KEY_F4','F5':'HID_KEY_F5','F6':'HID_KEY_F6','F7':'HID_KEY_F7','F8':'HID_KEY_F8','F9':'HID_KEY_F9','F10':'HID_KEY_F10','F11':'HID_KEY_F11','F12':'HID_KEY_F12', 'Enter':'HID_KEY_ENTER','Escape':'HID_KEY_ESCAPE','Backspace':'HID_KEY_BACKSPACE','Tab':'HID_KEY_TAB','Space':'HID_KEY_SPACE','Minus':'HID_KEY_MINUS','Equal':'HID_KEY_EQUAL','BracketLeft':'HID_KEY_BRACKET_LEFT','BracketRight':'HID_KEY_BRACKET_RIGHT','Backslash':'HID_KEY_BACKSLASH','Semicolon':'HID_KEY_SEMICOLON','Quote':'HID_KEY_APOSTROPHE','Backquote':'HID_KEY_GRAVE','Comma':'HID_KEY_COMMA','Period':'HID_KEY_PERIOD','Slash':'HID_KEY_SLASH', 'CapsLock':'HID_KEY_CAPS_LOCK','ScrollLock':'HID_KEY_SCROLL_LOCK','Pause':'HID_KEY_PAUSE','Insert':'HID_KEY_INSERT','Home':'HID_KEY_HOME','PageUp':'HID_KEY_PAGE_UP','Delete':'HID_KEY_DELETE','End':'HID_KEY_END','PageDown':'HID_KEY_PAGE_DOWN', 'ArrowRight':'HID_KEY_ARROW_RIGHT','ArrowLeft':'HID_KEY_ARROW_LEFT','ArrowDown':'HID_KEY_ARROW_DOWN','ArrowUp':'HID_KEY_ARROW_UP', 'ControlLeft':'HID_KEY_CONTROL_LEFT','ShiftLeft':'HID_KEY_SHIFT_LEFT','AltLeft':'HID_KEY_ALT_LEFT','MetaLeft':'HID_KEY_GUI_LEFT', 'ControlRight':'HID_KEY_CONTROL_RIGHT','ShiftRight':'HID_KEY_SHIFT_RIGHT','AltRight':'HID_KEY_ALT_RIGHT','MetaRight':'HID_KEY_GUI_RIGHT', 'IntlYen': 'HID_KEY_INTERNATIONAL1', 'IntlRo': 'HID_KEY_INTERNATIONAL3' };
    const physical_layout_order_name = [ "sw1", "sw62", "sw2", "sw5", "sw9", "sw13", "sw17", "sw21", "sw25", "sw30", "sw35", "sw40", "sw45", "sw50", "sw55", "sw59", "sw63", "sw3", "sw6", "sw10", "sw14", "sw18", "sw22", "sw26", "sw31", "sw36", "sw41", "sw46", "sw51", "sw56", "sw60", "sw4", "sw7", "sw11", "sw15", "sw19", "sw23", "sw27", "sw32", "sw37", "sw42", "sw47", "sw52", "sw57", "sw61", "sw8", "sw12", "sw16", "sw20", "sw24", "sw28", "sw33", "sw38", "sw43", "sw48", "sw53", "sw58", "sw29", "sw34", "sw39", "sw44", "sw49", "sw54", "D10", "D6" ];
    
    let keymapData = [
      [ 'KC_MS_BTN1', 'KC_MS_BTN2', 'HID_KEY_ESCAPE', 'HID_KEY_1', 'HID_KEY_2', 'HID_KEY_3', 'HID_KEY_4', 'HID_KEY_5', 'HID_KEY_6', 'HID_KEY_7', 'HID_KEY_8', 'HID_KEY_9', 'HID_KEY_0', 'HID_KEY_MINUS', 'HID_KEY_EQUAL', 'HID_KEY_INTERNATIONAL1', 'HID_KEY_DELETE', 'HID_KEY_GRAVE', 'HID_KEY_Q', 'HID_KEY_W', 'HID_KEY_E', 'HID_KEY_R', 'HID_KEY_T', 'HID_KEY_Y', 'HID_KEY_U', 'HID_KEY_I', 'HID_KEY_O', 'HID_KEY_P', 'HID_KEY_BRACKET_LEFT', 'HID_KEY_BRACKET_RIGHT', 'HID_KEY_BACKSPACE', 'HID_KEY_TAB', 'HID_KEY_A', 'HID_KEY_S', 'HID_KEY_D', 'HID_KEY_F', 'HID_KEY_G', 'HID_KEY_H', 'HID_KEY_J', 'HID_KEY_K', 'HID_KEY_L', 'HID_KEY_SEMICOLON', 'HID_KEY_APOSTROPHE', 'HID_KEY_BACKSLASH', 'HID_KEY_ENTER', 'HID_KEY_Z', 'HID_KEY_X', 'HID_KEY_C', 'HID_KEY_V', 'HID_KEY_B', 'HID_KEY_N', 'HID_KEY_M', 'HID_KEY_COMMA', 'HID_KEY_PERIOD', 'HID_KEY_SLASH', 'HID_KEY_INTERNATIONAL3', 'HID_KEY_SHIFT_LEFT', 'HID_KEY_SPACE', 'HID_KEY_GUI_LEFT', 'HID_KEY_ALT_LEFT', 'HID_KEY_CONTROL_LEFT', 'KC_MS_BTN3', 'L_LOWER', 'KC_MS_BTN4', 'KC_MS_BTN5'],
      [ 'KC_TRNS', 'KC_TRNS', 'SW_USB_BT', 'HID_KEY_F1', 'HID_KEY_F2', 'HID_KEY_F3', 'HID_KEY_F4', 'HID_KEY_F5', 'HID_KEY_F6', 'HID_KEY_F7', 'HID_KEY_F8', 'HID_KEY_F9', 'HID_KEY_F10', 'HID_KEY_F11', 'HID_KEY_F12', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'HID_KEY_ARROW_UP', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'HID_KEY_ARROW_LEFT', 'HID_KEY_ARROW_DOWN', 'HID_KEY_ARROW_RIGHT', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'HID_KEY_END', 'HID_KEY_HOME', 'HID_KEY_PAGE_UP', 'HID_KEY_PAGE_DOWN', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'KC_TRNS', 'L_LOWER', 'KC_RESET_KM', 'KC_REBOOT_DEF']
    ];

    const KEYCODE_DISPLAY_MAP = {'KC_MS_BTN1': 'L Click', 'KC_MS_BTN2': 'R Click', 'KC_MS_BTN3': 'M Click', 'KC_MS_BTN4': 'Back', 'KC_MS_BTN5': 'Fwd', 'KC_MS_UP': 'M Up', 'KC_MS_DOWN': 'M Down', 'KC_MS_LEFT': 'M Left', 'KC_MS_RIGHT': 'M Right', 'HID_KEY_CONTROL_LEFT': 'L Ctrl', 'HID_KEY_SHIFT_LEFT': 'L Shift', 'HID_KEY_ALT_LEFT': 'L Alt', 'HID_KEY_GUI_LEFT': 'L GUI', 'HID_KEY_CONTROL_RIGHT': 'R Ctrl', 'HID_KEY_SHIFT_RIGHT': 'R Shift', 'HID_KEY_ALT_RIGHT': 'R Alt', 'HID_KEY_GUI_RIGHT': 'R GUI', 'HID_KEY_ENTER': 'Enter', 'HID_KEY_ESCAPE': 'Esc', 'HID_KEY_BACKSPACE': 'BSPC', 'HID_KEY_DELETE': 'Del', 'HID_KEY_TAB': 'Tab', 'HID_KEY_SPACE': 'Space', 'HID_KEY_APPLICATION': 'App', 'HID_KEY_CAPS_LOCK': 'Caps', 'HID_KEY_ARROW_RIGHT': '→', 'HID_KEY_ARROW_LEFT': '←', 'HID_KEY_ARROW_DOWN': '↓', 'HID_KEY_ARROW_UP': '↑', 'HID_KEY_INSERT': 'Ins', 'HID_KEY_HOME': 'Home', 'HID_KEY_END': 'End', 'HID_KEY_PAGE_UP': 'PgUp', 'HID_KEY_PAGE_DOWN': 'PgDn', 'HID_KEY_PRINT_SCREEN': 'PrtSc', 'HID_KEY_SCROLL_LOCK': 'ScLk', 'HID_KEY_PAUSE': 'Pause', 'HID_KEY_MINUS': '-', 'HID_KEY_EQUAL': '=', 'HID_KEY_BRACKET_LEFT': '[', 'HID_KEY_BRACKET_RIGHT': ']', 'HID_KEY_BACKSLASH': '\\', 'HID_KEY_SEMICOLON': ';', 'HID_KEY_APOSTROPHE': "'", 'HID_KEY_GRAVE': '`', 'HID_KEY_COMMA': ',', 'HID_KEY_PERIOD': '.', 'HID_KEY_SLASH': '/', 'KC_TRNS': '▽', 'KC_NO': '×', 'L_LOWER': 'Lower', 'SW_USB_BT': 'Mode', 'KC_RESET_KM': 'Reset KM', 'KC_REBOOT_DEF': 'Boot DEF', 'HID_KEY_LANG1': 'Lang1', 'HID_KEY_LANG2': 'Lang2', 'HID_KEY_INTERNATIONAL1': 'Intl1', 'HID_KEY_INTERNATIONAL3': 'Intl3'};
    const KEYCODE_TO_VALUE_MAP = { "KC_NO": 0, "KC_TRNS": 0, "HID_KEY_A": 4, "HID_KEY_B": 5, "HID_KEY_C": 6, "HID_KEY_D": 7, "HID_KEY_E": 8, "HID_KEY_F": 9, "HID_KEY_G": 10, "HID_KEY_H": 11, "HID_KEY_I": 12, "HID_KEY_J": 13, "HID_KEY_K": 14, "HID_KEY_L": 15, "HID_KEY_M": 16, "HID_KEY_N": 17, "HID_KEY_O": 18, "HID_KEY_P": 19, "HID_KEY_Q": 20, "HID_KEY_R": 21, "HID_KEY_S": 22, "HID_KEY_T": 23, "HID_KEY_U": 24, "HID_KEY_V": 25, "HID_KEY_W": 26, "HID_KEY_X": 27, "HID_KEY_Y": 28, "HID_KEY_Z": 29, "HID_KEY_1": 30, "HID_KEY_2": 31, "HID_KEY_3": 32, "HID_KEY_4": 33, "HID_KEY_5": 34, "HID_KEY_6": 35, "HID_KEY_7": 36, "HID_KEY_8": 37, "HID_KEY_9": 38, "HID_KEY_0": 39, "HID_KEY_ENTER": 40, "HID_KEY_ESCAPE": 41, "HID_KEY_BACKSPACE": 42, "HID_KEY_TAB": 43, "HID_KEY_SPACE": 44, "HID_KEY_MINUS": 45, "HID_KEY_EQUAL": 46, "HID_KEY_BRACKET_LEFT": 47, "HID_KEY_BRACKET_RIGHT": 48, "HID_KEY_BACKSLASH": 49, "HID_KEY_SEMICOLON": 51, "HID_KEY_APOSTROPHE": 52, "HID_KEY_GRAVE": 53, "HID_KEY_COMMA": 54, "HID_KEY_PERIOD": 55, "HID_KEY_SLASH": 56, "HID_KEY_CAPS_LOCK": 57, "HID_KEY_F1": 58, "HID_KEY_F2": 59, "HID_KEY_F3": 60, "HID_KEY_F4": 61, "HID_KEY_F5": 62, "HID_KEY_F6": 63, "HID_KEY_F7": 64, "HID_KEY_F8": 65, "HID_KEY_F9": 66, "HID_KEY_F10": 67, "HID_KEY_F11": 68, "HID_KEY_F12": 69, "HID_KEY_PRINT_SCREEN": 70, "HID_KEY_SCROLL_LOCK": 71, "HID_KEY_PAUSE": 72, "HID_KEY_INSERT": 73, "HID_KEY_HOME": 74, "HID_KEY_PAGE_UP": 75, "HID_KEY_DELETE": 76, "HID_KEY_END": 77, "HID_KEY_PAGE_DOWN": 78, "HID_KEY_ARROW_RIGHT": 79, "HID_KEY_ARROW_LEFT": 80, "HID_KEY_ARROW_DOWN": 81, "HID_KEY_ARROW_UP": 82, "HID_KEY_CONTROL_LEFT": 224, "HID_KEY_SHIFT_LEFT": 225, "HID_KEY_ALT_LEFT": 226, "HID_KEY_GUI_LEFT": 227, "HID_KEY_CONTROL_RIGHT": 228, "HID_KEY_SHIFT_RIGHT": 229, "HID_KEY_ALT_RIGHT": 230, "HID_KEY_GUI_RIGHT": 231, "HID_KEY_APPLICATION": 101, "HID_KEY_LANG1": 144, "HID_KEY_LANG2": 145, "HID_KEY_INTERNATIONAL1": 135, "HID_KEY_INTERNATIONAL3": 137, "KC_MS_UP": 513, "KC_MS_DOWN": 514, "KC_MS_LEFT": 515, "KC_MS_RIGHT": 516, "KC_MS_BTN1": 517, "KC_MS_BTN2": 518, "KC_MS_BTN3": 519, "KC_MS_BTN4": 520, "KC_MS_BTN5": 521, "L_LOWER": 769, "SW_USB_BT": 1025, "KC_RESET_KM": 1281, "KC_REBOOT_DEF": 1282 };
    
    const VALUE_TO_KEYCODE_MAP = {};
    for (const [key, value] of Object.entries(KEYCODE_TO_VALUE_MAP)) {
        if (value !== 0 || key === "KC_NO") {
            VALUE_TO_KEYCODE_MAP[value] = key;
        }
    }
    
    const KEYMAP_SERVICE_UUID = 'adaf0001-c332-42a8-93bd-25e905756cb8';
    const KEYMAP_CHAR_UUID = 'adaf0002-c332-42a8-93bd-25e905756cb8';
    const CONFIG_CHAR_UUID = 'adaf0003-c332-42a8-93bd-25e905756cb8';
    const BATTERY_SERVICE_UUID = '0000180f-0000-1000-8000-00805f9b34fb';
    const BATTERY_CHAR_UUID = '00002a19-0000-1000-8000-00805f9b34fb';
    
    let currentLayer = 0; 
    let selectedKeyIndices = new Set(); 
    let keymapCharacteristic = null;
    let configCharacteristic = null;
    let batteryCharacteristic = null;
    let bluetoothDevice = null;
    let serialPort = null;
    let isWaitingForKeypress = false; 
    let targetKeyIndexForCapture = null;
    let connectionType = null;
    let batteryCheckInterval = null;
    
    const keyboardContainer = document.getElementById('keyboard-container');
    const layerSelector = document.getElementById('layer-selector');
    const generateButton = document.getElementById('generate-button');
    const fileImporter = document.getElementById('file-importer');
    const tabButtonsContainer = document.getElementById('tab-buttons-container');
    const tabContentContainer = document.getElementById('tab-content-container');
    const paletteDescription = document.getElementById('palette-description');
    const keyDescription = document.getElementById('key-description');
    const connectButton = document.getElementById('connect-button');
    const disconnectButton = document.getElementById('disconnect-button');
    const connectUsbButton = document.getElementById('connect-usb-button');
    const disconnectUsbButton = document.getElementById('disconnect-usb-button');
    const saveToKeyboardButton = document.getElementById('save-to-keyboard-button');
    const loadFromKeyboardButton = document.getElementById('load-from-keyboard-button');
    const connectionStatus = document.getElementById('connection-status');
    const batteryInfo = document.getElementById('battery-info');
    const debugLog = document.getElementById('debug-log');
    const jisLayoutToggle = document.getElementById('jis-layout-toggle');

    function addDebugLog(message) { 
        const timestamp = new Date().toLocaleTimeString(); 
        debugLog.innerHTML = `[${timestamp}] ${message}<br>` + debugLog.innerHTML; 
    }
    
    document.querySelectorAll('.main-tab-button').forEach(button => {
        button.addEventListener('click', () => {
            const targetTab = button.dataset.tab;
            document.querySelectorAll('.main-tab-button').forEach(btn => btn.classList.remove('active'));
            document.querySelectorAll('.main-tab-content').forEach(content => content.classList.remove('active'));
            button.classList.add('active');
            document.getElementById(`${targetTab}-tab`).classList.add('active');
        });
    });
    
    function startBatteryMonitoring() {
        if (batteryCheckInterval) clearInterval(batteryCheckInterval);
        getBatteryLevel();
        batteryCheckInterval = setInterval(() => {
            if (connectionType === 'bluetooth' || connectionType === 'usb') getBatteryLevel();
        }, 30000);
    }
    
    function stopBatteryMonitoring() {
        if (batteryCheckInterval) {
            clearInterval(batteryCheckInterval);
            batteryCheckInterval = null;
        }
    }
    
    async function getBatteryLevel() {
        if (!serialPort && !batteryCharacteristic) return;
        
        try {
            if (connectionType === 'bluetooth') {
                if (batteryCharacteristic) {
                    const batteryValue = await batteryCharacteristic.readValue();
                    const batteryLevel = batteryValue.getUint8(0);
                    batteryInfo.textContent = `🔋 Battery: ${batteryLevel}%`;
                }
            } else if (connectionType === 'usb' && serialPort && serialPort.writable) {
                const writer = serialPort.writable.getWriter();
                const encoder = new TextEncoder();
                await writer.write(encoder.encode('GET_BATTERY\n'));
                writer.releaseLock();
                
                const reader = serialPort.readable.getReader();
                const decoder = new TextDecoder();
                let response = '';
                const timeout = setTimeout(() => {
                    try { reader.cancel(); } catch(e) {}
                }, 2000);
                
                try {
                    while (true) {
                        const { value, done } = await reader.read();
                        if (done) break;
                        response += decoder.decode(value, { stream: true });
                        if (response.includes('\n')) {
                            clearTimeout(timeout);
                            break;
                        }
                    }
                } catch (e) {
                    addDebugLog('バッテリー読み取りタイムアウトまたはキャンセル');
                } finally {
                    try{ reader.releaseLock(); } catch(e){}
                }
                
                const match = response.match(/BATTERY:(\d+)/);
                if (match) {
                    const batteryLevel = parseInt(match[1]);
                    batteryInfo.textContent = `🔋 Battery: ${batteryLevel}%`;
                }
            }
        } catch (error) {
            addDebugLog('バッテリー取得エラー: ' + error.message);
        }
    }
    
    document.querySelectorAll('.connection-tab').forEach(tab => {
        tab.addEventListener('click', () => {
            const type = tab.dataset.type;
            document.querySelectorAll('.connection-tab').forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            
            if (type === 'bluetooth') {
                document.getElementById('bluetooth-section').style.display = 'block';
                document.getElementById('usb-section').style.display = 'none';
                document.getElementById('bt-help-text').style.display = 'block';
                document.getElementById('usb-help-text').style.display = 'none';
            } else {
                document.getElementById('bluetooth-section').style.display = 'none';
                document.getElementById('usb-section').style.display = 'block';
                document.getElementById('bt-help-text').style.display = 'none';
                document.getElementById('usb-help-text').style.display = 'block';
            }
        });
    });
    
    function getDisplayKeycode(keycode) {
        const useJisLayout = jisLayoutToggle.checked;
        if (useJisLayout && JIS_KEY_MAP[keycode]) return JIS_KEY_MAP[keycode];
        if (KEYCODE_DISPLAY_MAP[keycode]) return KEYCODE_DISPLAY_MAP[keycode];
        if (keycode.startsWith('HID_KEY_')) {
            const key = keycode.replace('HID_KEY_', '');
            if (key.startsWith('F') && !isNaN(key.substring(1))) return key;
            return key.length === 1 ? key : key.charAt(0).toUpperCase();
        }
        return keycode;
    }
    
    function renderKeyboard() {
        const physicalLayoutJSON = [ [{"w":1.5},"sw1",{"x":12,"w":1.5},"sw62"], ["sw2","sw5","sw9","sw13","sw17","sw21","sw25","sw30","sw35","sw40","sw45","sw50","sw55","sw59","sw63"], [{"w":1.5},"sw3","sw6","sw10","sw14","sw18","sw22","sw26","sw31","sw36","sw41","sw46","sw51","sw56",{"w":1.5},"sw60"], [{"w":1.75},"sw4","sw7","sw11","sw15","sw19","sw23","sw27","sw32","sw37","sw42","sw47","sw52","sw57",{"w":1.25},"sw61"], [{"x":2.1},"sw8","sw12","sw16","sw20","sw24","sw28","sw33","sw38","sw43","sw48","sw53",{"w":1.5},"sw58"], [{"y":-0.5},"D10","D6"], [{"y":-0.5,"x":6.25,"w":2.5},"sw29","sw34","sw39","sw44","sw49","sw54"] ];
        keyboardContainer.innerHTML = '';
        const baseKeySize = 42; const baseGap = 5;
        physicalLayoutJSON.forEach(rowItems => {
            const row = document.createElement('div'); row.className = 'key-row';
            let currentProps = { w: 1, x: 0, y: 0 };
            rowItems.forEach(item => {
                if (typeof item === 'object' && item !== null) {
                    Object.assign(currentProps, item);
                    if (currentProps.y !== 0) row.style.marginTop = `${currentProps.y * (baseKeySize + baseGap)}px`;
                } else if (typeof item === 'string') {
                    const keyName = item;
                    const keyIndex = physical_layout_order_name.indexOf(keyName);
                    if (keyIndex === -1) return;
                    const keycode = keymapData[currentLayer][keyIndex];
                    const keyEl = document.createElement('div'); keyEl.className = 'key'; keyEl.dataset.index = keyIndex;
                    if (selectedKeyIndices.has(keyIndex)) keyEl.classList.add('selected');
                    const codeEl = document.createElement('div'); codeEl.className = 'key-code';
                    if (isWaitingForKeypress && keyIndex === targetKeyIndexForCapture) {
                        keyEl.classList.add('waiting-capture');
                        codeEl.textContent = 'PRESS KEY';
                    } else {
                        codeEl.textContent = getDisplayKeycode(keycode);
                    }
                    keyEl.style.width = `${baseKeySize * currentProps.w + baseGap * (currentProps.w - 1)}px`;
                    if (currentProps.x > 0) keyEl.style.marginLeft = `${currentProps.x * (baseKeySize + baseGap)}px`;
                    const nameEl = document.createElement('div'); nameEl.className = 'key-name'; nameEl.textContent = keyName;
                    keyEl.appendChild(nameEl); keyEl.appendChild(codeEl);
                    keyEl.addEventListener('click', (e) => { const idx = parseInt(e.currentTarget.dataset.index); if (e.ctrlKey || e.metaKey) { selectedKeyIndices.has(idx) ? selectedKeyIndices.delete(idx) : selectedKeyIndices.add(idx); } else { selectedKeyIndices.clear(); selectedKeyIndices.add(idx); } renderKeyboard(); });
                    keyEl.addEventListener('dblclick', (e) => { const idx = parseInt(e.currentTarget.dataset.index); isWaitingForKeypress = true; targetKeyIndexForCapture = idx; renderKeyboard(); });
                    row.appendChild(keyEl);
                    currentProps = { w: 1, x: 0, y: currentProps.y };
                }
            });
            keyboardContainer.appendChild(row);
        });
    }

    function renderPalette() {
        tabButtonsContainer.innerHTML = ''; tabContentContainer.innerHTML = '';
        Object.keys(KEYCODES).forEach((category, idx) => {
            const btn = document.createElement('button'); btn.className = 'tab-button'; btn.textContent = category;
            if (idx === 0) { btn.classList.add('active'); paletteDescription.textContent = CATEGORY_DESCRIPTIONS[category]; }
            btn.addEventListener('click', () => { document.querySelectorAll('.tab-button').forEach(b => b.classList.remove('active')); document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active')); btn.classList.add('active'); document.getElementById(`tab-${idx}`).classList.add('active'); paletteDescription.textContent = CATEGORY_DESCRIPTIONS[category]; });
            tabButtonsContainer.appendChild(btn);
            const content = document.createElement('div'); content.className = 'tab-content'; content.id = `tab-${idx}`;
            if (idx === 0) content.classList.add('active');
            const keysContainer = document.createElement('div'); keysContainer.className = 'palette-keys';
            KEYCODES[category].forEach(keycode => {
                const keyBtn = document.createElement('button'); keyBtn.className = 'palette-key'; keyBtn.textContent = getDisplayKeycode(keycode); keyBtn.title = keycode;
                keyBtn.addEventListener('mouseenter', () => { keyDescription.innerHTML = KEYCODE_DESCRIPTIONS[keycode] || '&nbsp;'; });
                keyBtn.addEventListener('mouseleave', () => { keyDescription.innerHTML = '&nbsp;'; });
                keyBtn.addEventListener('click', () => { if (selectedKeyIndices.size === 0) { alert('Please select at least one key on the layout first!'); return; } selectedKeyIndices.forEach(idx => { keymapData[currentLayer][idx] = keycode; }); renderKeyboard(); });
                keysContainer.appendChild(keyBtn);
            });
            content.appendChild(keysContainer);
            tabContentContainer.appendChild(content);
        });
    }

    function handleKeyCapture(e) {
        if (!isWaitingForKeypress) return;
        e.preventDefault();
        if (e.code === 'Escape') { isWaitingForKeypress = false; targetKeyIndexForCapture = null; renderKeyboard(); return; }
        const mappedKeycode = EVENT_CODE_TO_KEYCODE_MAP[e.code];
        if (mappedKeycode) { keymapData[currentLayer][targetKeyIndexForCapture] = mappedKeycode; } 
        else { console.warn(`Unsupported key captured: ${e.code}`); alert(`このキー (${e.code}) はサポートされていません。`); }
        isWaitingForKeypress = false; targetKeyIndexForCapture = null; renderKeyboard();
    }
    
    function generateFile() {
        const settings = {
            MOUSE_HIGH_SPEED: document.getElementById('MOUSE_HIGH_SPEED').value,
            MOUSE_LOW_SPEED: document.getElementById('MOUSE_LOW_SPEED').value,
            MOUSE_ACCEL_THRESHOLD: document.getElementById('MOUSE_ACCEL_THRESHOLD').value,
            STICK_CENTER_X: document.getElementById('STICK_CENTER_X').value,
            STICK_CENTER_Y: document.getElementById('STICK_CENTER_Y').value,
            STICK_DEADZONE: document.getElementById('STICK_DEADZONE').value,
            SMOOTHING_SAMPLES: document.getElementById('SMOOTHING_SAMPLES').value,
            SCROLL_MIN_THRESHOLD: document.getElementById('SCROLL_MIN_THRESHOLD').value,
            SCROLL_MAX_THRESHOLD: document.getElementById('SCROLL_MAX_THRESHOLD').value,
            SCROLL_MIN_SPEED: document.getElementById('SCROLL_MIN_SPEED').value,
            SCROLL_MAX_SPEED: document.getElementById('SCROLL_MAX_SPEED').value,
            SCROLL_INTERVAL_MS: document.getElementById('SCROLL_INTERVAL_MS').value,
            MOUSE_HIGH_SPEED_BT: document.getElementById('MOUSE_HIGH_SPEED_BT').value,
            SCROLL_MIN_SPEED_BT: document.getElementById('SCROLL_MIN_SPEED_BT').value,
            SCROLL_MAX_SPEED_BT: document.getElementById('SCROLL_MAX_SPEED_BT').value,
            SLEEP_TIMEOUT: document.getElementById('SLEEP_TIMEOUT').value,
            LED_BRIGHTNESS: document.getElementById('LED_BRIGHTNESS').value,
            BLINK_INTERVAL_MS: document.getElementById('BLINK_INTERVAL_MS').value
        };
        
        let fileContent = `// =========================================================================
// ★★★ あなたが編集するのは、このファイルだけ! ★★★
// =========================================================================

#include <Adafruit_TinyUSB.h>

// --- アナログスティック設定 (USB用) ---
#define STICK_CENTER_X       ${settings.STICK_CENTER_X}
#define STICK_CENTER_Y       ${settings.STICK_CENTER_Y}
#define STICK_DEADZONE       ${settings.STICK_DEADZONE}
#define MOUSE_HIGH_SPEED     ${settings.MOUSE_HIGH_SPEED}f // ★ 最高速度
#define MOUSE_LOW_SPEED      ${settings.MOUSE_LOW_SPEED}f  // ★ 低速ゾーンの最高速度
#define SMOOTHING_SAMPLES    ${settings.SMOOTHING_SAMPLES}
#define MOUSE_ACCEL_THRESHOLD ${settings.MOUSE_ACCEL_THRESHOLD} // ★ 低速ゾーンが終わる傾き

// --- スクロール設定 (USB用) ---
#define SCROLL_MIN_THRESHOLD ${settings.SCROLL_MIN_THRESHOLD}
#define SCROLL_MAX_THRESHOLD ${settings.SCROLL_MAX_THRESHOLD}
#define SCROLL_MIN_SPEED ${settings.SCROLL_MIN_SPEED}f
#define SCROLL_MAX_SPEED ${settings.SCROLL_MAX_SPEED}f
#define SCROLL_INTERVAL_MS ${settings.SCROLL_INTERVAL_MS}

// --- Bluetooth専用設定 ---
#define MOUSE_HIGH_SPEED_BT  ${settings.MOUSE_HIGH_SPEED_BT}f  // ★ Bluetooth時のマウス最高速度
#define SCROLL_MIN_SPEED_BT  ${settings.SCROLL_MIN_SPEED_BT}f  // ★ Bluetooth時のスクロール最小速度
#define SCROLL_MAX_SPEED_BT  ${settings.SCROLL_MAX_SPEED_BT}f   // ★ Bluetooth時のスクロール最大速度

// --- 電源管理設定 ---
#define SLEEP_TIMEOUT ${settings.SLEEP_TIMEOUT}UL // スリープまでの時間(ms)
#define LED_BRIGHTNESS ${settings.LED_BRIGHTNESS}
#define BLINK_INTERVAL_MS ${settings.BLINK_INTERVAL_MS}

// --- キーマトリクス設定 ---
#define ROWS 8
#define COLS 8
#define LAYOUT_KEY_COUNT 65

// --- カスタムキーコード定義 ---
#define KC_MS_UP         0x201
#define KC_MS_DOWN       0x202
#define KC_MS_LEFT       0x203
#define KC_MS_RIGHT      0x204
#define KC_MS_BTN1       0x205 // 左クリック
#define KC_MS_BTN2       0x206 // 右クリック
#define KC_MS_BTN3       0x207 // 中クリック
#define KC_MS_BTN4       0x208 // 戻る
#define KC_MS_BTN5       0x209 // 進む
#define MOUSE_MOVE_SPEED 8
#define KC_NO            0x000
#define SW_USB_BT        0x401

#define KC_RESET_KM      0x501
#define KC_REBOOT_DEF    0x502

// JP Keyboard specific keycodes
#ifndef HID_KEY_INTERNATIONAL1
  #define HID_KEY_INTERNATIONAL1 0x87
#endif
#ifndef HID_KEY_INTERNATIONAL3
  #define HID_KEY_INTERNATIONAL3 0x89
#endif
#ifndef HID_KEY_LANG1
  #define HID_KEY_LANG1 0x90
#endif
#ifndef HID_KEY_LANG2
  #define HID_KEY_LANG2 0x91
#endif

// レイヤー機能用
#define NUM_LAYERS 2
#define L_LOWER    0x301
#define KC_TRNS    0x000


// --- キーマップレイアウト定義 ---
const uint16_t layout[NUM_LAYERS][LAYOUT_KEY_COUNT] = {
`;
        const keymapInValues = keymapData.map(layer => {
            return layer.map(kc => KEYCODE_TO_VALUE_MAP[kc] !== undefined ? `0x${KEYCODE_TO_VALUE_MAP[kc].toString(16).toUpperCase()}`: "0x000");
        });

        keymapInValues.forEach((layer, layerIndex) => {
            fileContent += `  // [${layerIndex}] = ${layerIndex === 0 ? 'Default Layer' : 'Lower Layer (Fnキーを押している間)'}\n  {\n`;
            let keyIdx = 0;
            const createRow = (count, comment) => {
                fileContent += `    // ${comment}\n    `;
                fileContent += layer.slice(keyIdx, keyIdx + count).join(', ') + ',\n';
                keyIdx += count;
            };
            const row_counts = { '1段目': 2, '2段目': 15, '3段目': 14, '4段目': 14, '5段目': 12, '6段目': 6, '直接接続キー': 2 };
            for(const [comment, count] of Object.entries(row_counts)) {
                createRow(count, comment);
            }
            fileContent = fileContent.slice(0, -2) + '\n'; // 最後のカンマを削除
            fileContent += '  }';
            fileContent += (layerIndex < keymapInValues.length - 1) ? ',\n\n' : '\n';
        });
        fileContent += `};\n`;
        const blob = new Blob([fileContent], { type: 'text/plain' });
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = 'keymap.h';
        a.click();
        URL.revokeObjectURL(a.href);
    }
    
    function parseKeymapH(content) { 
        try { 
            const patterns = {
                MOUSE_HIGH_SPEED: /#define\s+MOUSE_HIGH_SPEED\s+([\d.]+)f?/,
                MOUSE_LOW_SPEED: /#define\s+MOUSE_LOW_SPEED\s+([\d.]+)f?/,
                MOUSE_ACCEL_THRESHOLD: /#define\s+MOUSE_ACCEL_THRESHOLD\s+(\d+)/,
                STICK_CENTER_X: /#define\s+STICK_CENTER_X\s+(\d+)/,
                STICK_CENTER_Y: /#define\s+STICK_CENTER_Y\s+(\d+)/,
                STICK_DEADZONE: /#define\s+STICK_DEADZONE\s+(\d+)/,
                SMOOTHING_SAMPLES: /#define\s+SMOOTHING_SAMPLES\s+(\d+)/,
                SCROLL_MIN_THRESHOLD: /#define\s+SCROLL_MIN_THRESHOLD\s+(\d+)/,
                SCROLL_MAX_THRESHOLD: /#define\s+SCROLL_MAX_THRESHOLD\s+(\d+)/,
                SCROLL_MIN_SPEED: /#define\s+SCROLL_MIN_SPEED\s+([\d.]+)f?/,
                SCROLL_MAX_SPEED: /#define\s+SCROLL_MAX_SPEED\s+([\d.]+)f?/,
                SCROLL_INTERVAL_MS: /#define\s+SCROLL_INTERVAL_MS\s+(\d+)/,
                MOUSE_HIGH_SPEED_BT: /#define\s+MOUSE_HIGH_SPEED_BT\s+([\d.]+)f?/,
                SCROLL_MIN_SPEED_BT: /#define\s+SCROLL_MIN_SPEED_BT\s+([\d.]+)f?/,
                SCROLL_MAX_SPEED_BT: /#define\s+SCROLL_MAX_SPEED_BT\s+([\d.]+)f?/,
                SLEEP_TIMEOUT: /#define\s+SLEEP_TIMEOUT\s+(\d+)UL/,
                LED_BRIGHTNESS: /#define\s+LED_BRIGHTNESS\s+(\d+)/,
                BLINK_INTERVAL_MS: /#define\s+BLINK_INTERVAL_MS\s+(\d+)/
            };
            
            for (const [key, pattern] of Object.entries(patterns)) {
                const match = content.match(pattern);
                if (match) {
                    document.getElementById(key).value = match[1];
                }
            }
            
            const keymapMatch = content.match(/const\s+uint16_t\s+layout\[.*?\]\[.*?\]\s*=\s*{([\s\S]*?)};/); 
            if (!keymapMatch) throw new Error("Could not find 'layout' array."); 
            let layersContent = keymapMatch[1]; 
            layersContent = layersContent.replace(/\/\*[\s\S]*?\*\/|\/\/.*/g, ''); 
            const layerMatches = layersContent.match(/{[^}]+}/g); 
            if (!layerMatches || layerMatches.length < 2) throw new Error("Could not parse layers."); 
            
            const newKeymapData = layerMatches.map(layerStr => { 
                const values = layerStr.replace(/{|}/g, '').split(',').map(s => parseInt(s.trim())).filter(s => !isNaN(s));
                if (values.length !== 65) throw new Error(`Incorrect number of keys found in a layer: ${values.length}`);
                return values.map(val => VALUE_TO_KEYCODE_MAP[val] || 'KC_NO');
            });

            keymapData = newKeymapData;
            renderKeyboard(); 
            alert('Keymap loaded successfully!'); 
        } catch (error) { 
            alert('Error parsing file: ' + error.message); 
            console.error(error); 
        } 
    }
    
    async function connectBluetooth() { 
        if (!navigator.bluetooth) { 
            alert('このブラウザはWeb Bluetoothに対応していません。Chrome/Edgeをご使用ください。'); 
            return; 
        } 
        try { 
            addDebugLog('BT接続開始...'); 
            connectionStatus.textContent = 'Status: デバイス選択中...'; 
            const options = { 
                acceptAllDevices: false, 
                filters: [ { name: 'PotaKB' }, { namePrefix: 'Pota' } ], 
                optionalServices: [ KEYMAP_SERVICE_UUID, BATTERY_SERVICE_UUID ] 
            }; 
            bluetoothDevice = await navigator.bluetooth.requestDevice(options); 
            bluetoothDevice.addEventListener('gattserverdisconnected', handleDisconnect); 
            connectionStatus.textContent = 'Status: 接続中...'; 
            const server = await bluetoothDevice.gatt.connect(); 
            const keymapService = await server.getPrimaryService(KEYMAP_SERVICE_UUID); 
            keymapCharacteristic = await keymapService.getCharacteristic(KEYMAP_CHAR_UUID);
            
            try {
                configCharacteristic = await keymapService.getCharacteristic(CONFIG_CHAR_UUID);
                addDebugLog('Config特性取得成功');
            } catch (e) {
                addDebugLog('Config特性取得失敗 - 古いファームウェアの可能性');
            }
            
            try {
                const batteryService = await server.getPrimaryService(BATTERY_SERVICE_UUID);
                batteryCharacteristic = await batteryService.getCharacteristic(BATTERY_CHAR_UUID);
            } catch (e) {
                addDebugLog('バッテリーサービス取得不可');
            }
            
            connectionStatus.textContent = `Status: 接続成功! (${bluetoothDevice.name})`; 
            connectionType = 'bluetooth';
            updateConnectionUI(true, 'bluetooth');
            startBatteryMonitoring();
        } catch(error) { 
            connectionStatus.textContent = 'Status: 接続失敗'; 
            addDebugLog('BT接続エラー: ' + error.message);
        } 
    }
    
    async function connectUSB() {
        if (!('serial' in navigator)) {
            alert('このブラウザはWeb Serial APIに対応していません。Chrome/Edgeをご使用ください。');
            return;
        }
        try {
            addDebugLog('USB接続開始...');
            connectionStatus.textContent = 'Status: ポート選択中...';
            
            serialPort = await navigator.serial.requestPort({
                filters: [{ usbVendorId: 0x239A, usbProductId: 0x8029 }]
            });

            serialPort.addEventListener('disconnect', handleDisconnect);
            
            connectionStatus.textContent = 'Status: ポートオープン中...';
            await serialPort.open({ baudRate: 115200 });
            
            await new Promise(resolve => setTimeout(resolve, 100));
            
            connectionStatus.textContent = 'Status: USB接続成功!';
            connectionType = 'usb';
            updateConnectionUI(true, 'usb');
            addDebugLog('USB接続完了');
            startBatteryMonitoring();
            
        } catch(error) {
            let errorMsg = error.message;
            if (errorMsg.includes("Failed to open serial port.")) {
                errorMsg += "\n\n対処法:\n1. Arduino IDEなどのシリアルモニタを閉じる\n2. 他のアプリでポートを使用していないか確認\n3. デバイスを再接続する";
            }
            connectionStatus.textContent = 'Status: USB接続失敗';
            addDebugLog('USB接続エラー: ' + errorMsg);
        }
    }
    
    async function disconnectBluetooth() {
        if (bluetoothDevice && bluetoothDevice.gatt.connected) {
            await bluetoothDevice.gatt.disconnect();
            addDebugLog('Bluetooth手動切断');
        }
    }
    
    async function disconnectUSB() {
        if (serialPort) {
            await serialPort.close();
            addDebugLog('USB手動切断');
        }
    }
    
    function handleDisconnect() {
        if (connectionType === 'usb') serialPort = null;
        if (connectionType === 'bluetooth') bluetoothDevice = null;
        
        connectionStatus.textContent = 'Status: 切断されました';
        keymapCharacteristic = null;
        configCharacteristic = null;
        batteryCharacteristic = null;
        connectionType = null;
        updateConnectionUI(false, null);
        stopBatteryMonitoring();
        batteryInfo.textContent = '🔋 Battery: --';
        addDebugLog('デバイス切断完了');
    }
    
    function updateConnectionUI(connected, type) {
        if (connected) {
            if (type === 'bluetooth') {
                connectButton.style.display = 'none';
                disconnectButton.style.display = 'block';
            } else if (type === 'usb') {
                connectUsbButton.style.display = 'none';
                disconnectUsbButton.style.display = 'block';
            }
            saveToKeyboardButton.disabled = false;
            loadFromKeyboardButton.disabled = false;
        } else {
            connectButton.style.display = 'block';
            disconnectButton.style.display = 'none';
            connectUsbButton.style.display = 'block';
            disconnectUsbButton.style.display = 'none';
            saveToKeyboardButton.disabled = true;
            loadFromKeyboardButton.disabled = true;
        }
    }
    
    async function loadFromKeyboard() {
        if (!keymapCharacteristic && !serialPort) {
            alert('先にデバイスに接続してください!');
            return;
        }
        
        try {
            connectionStatus.textContent = 'Status: 読み込み中...';
            
            // Step 1: キーマップを読み込む
            addDebugLog('キーマップ読み込み開始...');
            let keymapBuffer;
            
            if (connectionType === 'bluetooth') {
                const dataValue = await keymapCharacteristic.readValue();
                keymapBuffer = dataValue.buffer;
            } else if (connectionType === 'usb') {
                const writer = serialPort.writable.getWriter();
                await writer.write(new TextEncoder().encode('READ_KEYMAP\n'));
                writer.releaseLock();
                
                const reader = serialPort.readable.getReader();
                const chunks = [];
                const expectedBytes = 2 * 65 * 2;
                let receivedBytes = 0;
                const timeout = setTimeout(() => { try{ reader.cancel(); } catch(e){} }, 3000);
                
                while(receivedBytes < expectedBytes) {
                    const { value, done } = await reader.read();
                    if(done) break;
                    chunks.push(value);
                    receivedBytes += value.length;
                }
                clearTimeout(timeout);
                reader.releaseLock();
                
                const allData = new Uint8Array(receivedBytes);
                let offset = 0;
                for(const chunk of chunks) {
                    allData.set(chunk, offset);
                    offset += chunk.length;
                }
                keymapBuffer = allData.buffer;
            }
            
            if (keymapBuffer.byteLength < 260) {
                throw new Error(`キーマップデータサイズが不足: ${keymapBuffer.byteLength} bytes`);
            }
            
            const keymapView = new DataView(keymapBuffer);
            const newKeymapData = [[], []];
            for (let layer = 0; layer < 2; layer++) {
                for (let key = 0; key < 65; key++) {
                    const offset = (layer * 65 + key) * 2;
                    const value = keymapView.getUint16(offset, true);
                    newKeymapData[layer].push(VALUE_TO_KEYCODE_MAP[value] || 'KC_NO');
                }
            }
            keymapData = newKeymapData;
            renderKeyboard();
            
            // Step 2: 設定を読み込む
            addDebugLog('設定読み込み開始...');
            let configBuffer;

            if (connectionType === 'bluetooth') {
                if (configCharacteristic) {
                    const data = await configCharacteristic.readValue();
                    configBuffer = data.buffer;
                } else {
                    addDebugLog('設定特性が利用不可 - スキップ');
                }
            } else if (connectionType === 'usb') {
                const writer = serialPort.writable.getWriter();
                await writer.write(new TextEncoder().encode('READ_CONFIG\n'));
                writer.releaseLock();
                
                const reader = serialPort.readable.getReader();
                const chunks = [];
                const expectedBytes = 56;
                let receivedBytes = 0;
                const timeout = setTimeout(() => { try{ reader.cancel(); } catch(e){} }, 3000);

                while(receivedBytes < expectedBytes) {
                    const { value, done } = await reader.read();
                    if(done) break;
                    chunks.push(value);
                    receivedBytes += value.length;
                }
                clearTimeout(timeout);
                reader.releaseLock();
                
                const allData = new Uint8Array(receivedBytes);
                let offset = 0;
                for(const chunk of chunks) {
                    allData.set(chunk, offset);
                    offset += chunk.length;
                }
                configBuffer = allData.buffer;
            }
            
            if(configBuffer) parseSettingsFromBytes(configBuffer);
            
            connectionStatus.textContent = 'Status: 読み込み完了!';
            alert('✓ キーマップと設定の読み込みに成功しました!');
            addDebugLog('全データ読み込み完了');
        } catch(error) {
            alert('読み込みに失敗しました: ' + error.message);
            connectionStatus.textContent = 'Status: 読み込み失敗';
            addDebugLog('読み込みエラー: ' + error.message);
        }
    }
    
    async function saveToKeyboard() { 
        if (!keymapCharacteristic && !serialPort) { 
            alert('先にデバイスに接続してください!'); 
            return; 
        } 
        let modeSwitchFound = false; 
        keymapData.forEach(layer => { 
            if (layer.includes('SW_USB_BT')) modeSwitchFound = true;
        }); 
        if (!modeSwitchFound) { 
            alert("エラー: 'Mode'キー (SW_USB_BT) がキーマップに配置されていません。\nUSB/BTの切り替えができなくなるため、書き込みを中止します。"); 
            return; 
        } 
        if (!keymapData.flat().some(k => k === 'KC_RESET_KM' || k === 'KC_REBOOT_DEF')) { 
            if (!confirm("警告:\nキーマップをリセットする手段がありません。このまま書き込みますか?")) return;
        } 
        if (!confirm('キーボードにキーマップと設定を書き込みます。書き込み後、キーボードは自動的に再起動します。\n\n続行しますか?')) return;
        
        try { 
            connectionStatus.textContent = 'Status: 書き込み中...'; 
            saveToKeyboardButton.disabled = true; 
            
            if (connectionType === 'usb') {
                const writer = serialPort.writable.getWriter();
                const encoder = new TextEncoder();
                try {
                    connectionStatus.textContent = 'Status: キーマップ書き込み中...';
                    const keymapBytes = convertKeymapToBytes(keymapData); 
                    await writer.write(encoder.encode('WRITE_KEYMAP\n'));
                    await new Promise(r => setTimeout(r, 100));
                    await writer.write(new Uint8Array(keymapBytes));
                    await new Promise(r => setTimeout(r, 300));
                    addDebugLog('USB経由でキーマップ書き込み完了');

                    connectionStatus.textContent = 'Status: 設定書き込み中...';
                    const settingsBytes = convertSettingsToBytes();
                    await writer.write(encoder.encode('WRITE_CONFIG\n'));
                    await new Promise(r => setTimeout(r, 100));
                    await writer.write(new Uint8Array(settingsBytes));
                    await new Promise(r => setTimeout(r, 300));
                    addDebugLog('USB経由で設定書き込み完了');

                    connectionStatus.textContent = 'Status: 再起動コマンド送信中...';
                    await writer.write(encoder.encode('REBOOT\n'));
                    addDebugLog('USB経由で再起動コマンド送信完了');
                } finally {
                    writer.releaseLock();
                }

            } else if (connectionType === 'bluetooth') {
                const keymapBytes = convertKeymapToBytes(keymapData); 
                const CHUNK_BT = 20; 
                for (let i = 0; i < Math.ceil(keymapBytes.byteLength / CHUNK_BT); i++) { 
                    const chunk = keymapBytes.slice(i * CHUNK_BT, (i + 1) * CHUNK_BT); 
                    await keymapCharacteristic.writeValueWithoutResponse(chunk); 
                    const progress = Math.round(((i + 1) / Math.ceil(keymapBytes.byteLength / CHUNK_BT)) * 50);
                    connectionStatus.textContent = `Status: キーマップ書き込み中... (${progress}%)`; 
                    await new Promise(r => setTimeout(r, 20)); 
                }

                if (configCharacteristic) {
                    const settingsBytes = convertSettingsToBytes();
                    for (let i = 0; i < Math.ceil(settingsBytes.byteLength / CHUNK_BT); i++) {
                        const chunk = settingsBytes.slice(i * CHUNK_BT, (i + 1) * CHUNK_BT);
                        await configCharacteristic.writeValueWithoutResponse(chunk);
                        const progress = 50 + Math.round(((i + 1) / Math.ceil(settingsBytes.byteLength / CHUNK_BT)) * 50);
                        connectionStatus.textContent = `Status: 設定書き込み中... (${progress}%)`;
                        await new Promise(r => setTimeout(r, 50));
                    }
                    addDebugLog('BT経由で設定書き込み完了');
                }
            }
            
            connectionStatus.textContent = 'Status: 書き込み完了! キーボードが再起動します'; 
            alert('✓ キーマップと設定の保存に成功しました!\n\nキーボードが再起動します。\n数秒後に再接続してください。'); 
            
            setTimeout(() => {
                if(connectionType === 'usb') disconnectUSB();
                else if (connectionType === 'bluetooth') disconnectBluetooth();
            }, 1000);
    
        } catch (error) { 
            alert('書き込みに失敗しました: ' + error.message); 
            connectionStatus.textContent = 'Status: 書き込み失敗'; 
            saveToKeyboardButton.disabled = false; 
            addDebugLog('書き込みエラー: ' + error.message);
        } 
    }
    
    function convertKeymapToBytes(stringKeymap) { 
        const buffer = new ArrayBuffer(2 * 65 * 2); 
        const view = new DataView(buffer); 
        stringKeymap.forEach((layer, layerIndex) => { 
            layer.forEach((keycodeStr, keyIndex) => { 
                const value = KEYCODE_TO_VALUE_MAP[keycodeStr] || 0; 
                view.setUint16((layerIndex * 65 + keyIndex) * 2, value, true); 
            }); 
        }); 
        return buffer; 
    }
    
    function convertSettingsToBytes() {
        const buffer = new ArrayBuffer(56);
        const view = new DataView(buffer);
        let offset = 0;
        view.setUint16(offset, parseInt(document.getElementById('STICK_CENTER_X').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('STICK_CENTER_Y').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('STICK_DEADZONE').value), true); offset += 2;
        view.setFloat32(offset, parseFloat(document.getElementById('MOUSE_HIGH_SPEED').value), true); offset += 4;
        view.setFloat32(offset, parseFloat(document.getElementById('MOUSE_LOW_SPEED').value), true); offset += 4;
        view.setUint16(offset, parseInt(document.getElementById('MOUSE_ACCEL_THRESHOLD').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('SMOOTHING_SAMPLES').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('SCROLL_MIN_THRESHOLD').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('SCROLL_MAX_THRESHOLD').value), true); offset += 2;
        view.setFloat32(offset, parseFloat(document.getElementById('SCROLL_MIN_SPEED').value), true); offset += 4;
        view.setFloat32(offset, parseFloat(document.getElementById('SCROLL_MAX_SPEED').value), true); offset += 4;
        view.setUint16(offset, parseInt(document.getElementById('SCROLL_INTERVAL_MS').value), true); offset += 2;
        view.setFloat32(offset, parseFloat(document.getElementById('MOUSE_HIGH_SPEED_BT').value), true); offset += 4;
        view.setFloat32(offset, parseFloat(document.getElementById('SCROLL_MIN_SPEED_BT').value), true); offset += 4;
        view.setFloat32(offset, parseFloat(document.getElementById('SCROLL_MAX_SPEED_BT').value), true); offset += 4;
        view.setUint32(offset, parseInt(document.getElementById('SLEEP_TIMEOUT').value), true); offset += 4;
        view.setUint16(offset, parseInt(document.getElementById('LED_BRIGHTNESS').value), true); offset += 2;
        view.setUint16(offset, parseInt(document.getElementById('BLINK_INTERVAL_MS').value), true); offset += 2;
        view.setUint32(offset, 0x504F5441, true);
        return buffer;
    }
    
    function parseSettingsFromBytes(buffer) {
        const isOldVersion = buffer.byteLength === 44;
        const isNewVersion = buffer.byteLength === 56;
        if (!isOldVersion && !isNewVersion) throw new Error(`設定データサイズが不正: ${buffer.byteLength} bytes`);
        
        const view = new DataView(buffer);
        const s = {};
        let offset = 0;
        
        s.STICK_CENTER_X = view.getUint16(offset, true); offset += 2;
        s.STICK_CENTER_Y = view.getUint16(offset, true); offset += 2;
        s.STICK_DEADZONE = view.getUint16(offset, true); offset += 2;
        s.MOUSE_HIGH_SPEED = view.getFloat32(offset, true).toFixed(1); offset += 4;
        s.MOUSE_LOW_SPEED = view.getFloat32(offset, true).toFixed(1); offset += 4;
        s.MOUSE_ACCEL_THRESHOLD = view.getUint16(offset, true); offset += 2;
        s.SMOOTHING_SAMPLES = view.getUint16(offset, true); offset += 2;
        s.SCROLL_MIN_THRESHOLD = view.getUint16(offset, true); offset += 2;
        s.SCROLL_MAX_THRESHOLD = view.getUint16(offset, true); offset += 2;
        s.SCROLL_MIN_SPEED = view.getFloat32(offset, true).toFixed(2); offset += 4;
        s.SCROLL_MAX_SPEED = view.getFloat32(offset, true).toFixed(2); offset += 4;
        s.SCROLL_INTERVAL_MS = view.getUint16(offset, true); offset += 2;
        
        if (isNewVersion) {
            s.MOUSE_HIGH_SPEED_BT = view.getFloat32(offset, true).toFixed(1); offset += 4;
            s.SCROLL_MIN_SPEED_BT = view.getFloat32(offset, true).toFixed(2); offset += 4;
            s.SCROLL_MAX_SPEED_BT = view.getFloat32(offset, true).toFixed(2); offset += 4;
        }
        
        s.SLEEP_TIMEOUT = view.getUint32(offset, true); offset += 4;
        s.LED_BRIGHTNESS = view.getUint16(offset, true); offset += 2;
        s.BLINK_INTERVAL_MS = view.getUint16(offset, true); offset += 2;

        Object.keys(s).forEach(key => {
            const el = document.getElementById(key);
            if (el) el.value = s[key];
        });
        
        addDebugLog('設定値のパース完了');
    }
    
    layerSelector.addEventListener('change', (e) => { currentLayer = parseInt(e.target.value); selectedKeyIndices.clear(); renderKeyboard(); });
    generateButton.addEventListener('click', generateFile);
    fileImporter.addEventListener('change', (e) => { const file = e.target.files[0]; if (!file) return; const reader = new FileReader(); reader.onload = (event) => { parseKeymapH(event.target.result); }; reader.readAsText(file); e.target.value = ''; });
    connectButton.addEventListener('click', connectBluetooth);
    disconnectButton.addEventListener('click', disconnectBluetooth);
    connectUsbButton.addEventListener('click', connectUSB);
    disconnectUsbButton.addEventListener('click', disconnectUSB);
    saveToKeyboardButton.addEventListener('click', saveToKeyboard);
    loadFromKeyboardButton.addEventListener('click', loadFromKeyboard);
    jisLayoutToggle.addEventListener('change', renderKeyboard);
    document.addEventListener('keydown', handleKeyCapture);

    renderPalette();
    renderKeyboard();
    
    document.getElementById('MOUSE_HIGH_SPEED').value = "3.6";
    document.getElementById('MOUSE_LOW_SPEED').value = "0.0";
    document.getElementById('MOUSE_ACCEL_THRESHOLD').value = "400";
    document.getElementById('STICK_CENTER_X').value = "470";
    document.getElementById('STICK_CENTER_Y').value = "470";
    document.getElementById('STICK_DEADZONE').value = "75";
    document.getElementById('SMOOTHING_SAMPLES').value = "2";
    document.getElementById('SCROLL_MIN_THRESHOLD').value = "80";
    document.getElementById('SCROLL_MAX_THRESHOLD').value = "350";
    document.getElementById('SCROLL_MIN_SPEED').value = "0.05";
    document.getElementById('SCROLL_MAX_SPEED').value = "0.20";
    document.getElementById('SCROLL_INTERVAL_MS').value = "10";
    document.getElementById('MOUSE_HIGH_SPEED_BT').value = "60.0";
    document.getElementById('SCROLL_MIN_SPEED_BT').value = "0.15";
    document.getElementById('SCROLL_MAX_SPEED_BT').value = "1.20";
    document.getElementById('SLEEP_TIMEOUT').value = "300000";
    document.getElementById('LED_BRIGHTNESS').value = "25";
    document.getElementById('BLINK_INTERVAL_MS').value = "600";
    
    addDebugLog('アプリケーション起動完了');
});