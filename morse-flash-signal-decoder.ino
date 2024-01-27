#include <stdint.h>
#include <U8g2lib.h>

/*---- debug print ----*/
#define TOSTR(x) #x
#define TOSTRX(x) TOSTR(x)
#define DEBUG_PRINT_VARIABLE(x) Serial.print(TOSTRX(x));Serial.print(":");Serial.println(x);
/*---- debug print ----*/


#define U8G2_ENABLE
#if defined(U8G2_ENABLE)
#define WIRE_FREQ 400*1000 /*fast mode*/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define PIN_SDA 21
#define PIN_SCL 22
#define SCREEN_LINE(n) ((16*(n))+15)
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
void screen_write_line(uint32_t line, char* msg) {
    String smsg = String(msg);
    screen_write_line(line, smsg);
}
static String smsg2="";
void screen_write_line(uint32_t line, String smsg) {
    smsg2 = smsg + "               ";
    u8g2.drawStr(0,SCREEN_LINE(line),(smsg2.substring(0, 16)).c_str());
    u8g2.sendBuffer();
}
#endif


const uint32_t pinOnboard = 0;
const uint32_t pinBuzz = 15;
const uint32_t chBuzz = 0;
const uint32_t pinPhoto = 32;
const uint32_t pinReset = 0;

const uint32_t interval_short = 100;
const uint32_t len_flash_long  = 240;
const uint32_t len_interval_short = 240;
const uint32_t len_interval_long = 2000;

static const float tone_base_do = 261.626;
static const float tone_base_re = 293.665;
static const float tone_base_mi = 329.628;
static const float tone_base_fa = 349.228;
static const float tone_base_so = 391.995;
static const float tone_base_ra = 440.000;
static const float tone_base_si = 493.883;

void setup() {
    Serial.begin( 115200 );
    Serial.println( "Hello!" );
    pinMode(pinBuzz, OUTPUT);
    pinMode(pinPhoto, INPUT);

    ledcSetup(chBuzz, 12000, 8);
    ledcAttachPin(pinBuzz, chBuzz);

#if defined(U8G2_ENABLE)
    u8g2.setBusClock(WIRE_FREQ);
    u8g2.begin();
    u8g2.setFlipMode(0);
    u8g2.setContrast(128);
    u8g2.setFont(u8g2_font_8x13B_mf);
    u8g2.clearBuffer();
    screen_write_line(0, "Welcome!!");
    screen_write_line(1, "Morse Decoder");
    delay(2000);
    screen_write_line(0, "");
    screen_write_line(1, "");
#endif
}

struct morse_pat {
    char letter;
    String pattern;
};
static morse_pat morse_table[] = {
    {'A',".-"},    // アホー
    {'B',"-..."},  // ビートルズ
    {'C',"-.-."},  // チーズケーキ
    {'D',"-.."},   // ドーナツ
    {'E',"."},     // エ (絵)
    {'F',"..-."},  // フラフープ
    {'G',"--."},   // ゴーカート
    {'H',"...."},  // ヘンタイ
    {'I',".."},    // アイ (愛)
    {'J',".---"},  // ジエーホーホー (自衛方法)
    {'K',"-.-"},   // ケーシチョー
    {'L',".-.."},  // レディーガガ
    {'M',"--"},    // メーター
    {'N',"-."},    // ノート
    {'O',"---"},   // オーヨーホー (応用法)
    {'P',".--."},  // プレーボール
    {'Q',"--.-"},  // キューキューシキュー (救急至急)
    {'R',".-."},   // リコール
    {'S',"..."},   // スシヤ （寿司屋)
    {'T',"-"},     // ティー
    {'U',"..-"},   // ウルセー
    {'V',"...-"},  // ビクトリー
    {'W',".--"},   // ワヨーチュー (和洋中)
    {'X',"-..-"},  // エークスレー (X線)
    {'Y',"-.--"},  // ヨークシャーシュー (州)
    {'Z',"--.."},  // ザーザーアメ
    {'1',".----"},
    {'2',"..---"},
    {'3',"...--"},
    {'4',"....-"},
    {'5',"....."},
    {'6',"-...."},
    {'7',"--..."},
    {'8',"---.."},
    {'9',"----."},
    {'0',"-----"},
    {'@',".--.-."}, // アトーオーマアーク
};

char find_morse(String pattern) {
    uint32_t morse_table_len = sizeof(morse_table)/sizeof(morse_pat);
    for (uint32_t i=0; i<morse_table_len; i++){
        if (morse_table[i].pattern == pattern) {
            return morse_table[i].letter;
        }
    }
    return '?';
}

static uint32_t photo_prev = HIGH;
static uint32_t millis_prev = 0;
static bool delimit_flash = true;
static bool delimit_word = true;
static String message = "";
static String pattern = "";
void loop() {
    uint32_t millis_curr = millis();
    uint32_t photo_curr = digitalRead(pinPhoto);
    if (photo_prev != photo_curr) {
        uint32_t millis_diff = millis_curr - millis_prev;
        if (photo_curr==HIGH) {
            //flash off
            ledcWriteTone(chBuzz, 0.0f);
            if (len_flash_long < millis_diff) {
                pattern += "-";
            } else {
                pattern += ".";
            }
            screen_write_line(0,pattern);
        } else {
            //flash on
            ledcWriteTone(chBuzz, tone_base_mi*3.0f);
        }
        photo_prev = photo_curr;
        millis_prev = millis_curr;
    } else {
        if (photo_curr==HIGH) {
            uint32_t millis_diff = millis_curr - millis_prev;
            if (!delimit_flash && millis_diff > len_interval_short) {
                delimit_flash = true;
                char letter = find_morse(pattern);
                message += String(letter);
                uint32_t msg_len = (message+".").length()-1;
                if (msg_len > 16) {
                    message = message.substring(msg_len-16);
                }
                screen_write_line(1,message);
                DEBUG_PRINT_VARIABLE(message);
                pattern = "";
            }
            if (!delimit_word && millis_diff > len_interval_long) {
                delimit_word = true;
                message += " ";
                uint32_t msg_len = (message+".").length()-1;
                if (msg_len > 16) {
                    message = message.substring(msg_len-16);
                }
                screen_write_line(1,message);
                DEBUG_PRINT_VARIABLE(message);
            }
        } else {
            delimit_flash = false;
            delimit_word = false;
        }
    }
    if (digitalRead(pinOnboard)==LOW) {
        message = "";
        pattern = "";
        screen_write_line(0,"");
        screen_write_line(1,"");
    }
#if defined(U8G2_ENABLE)
    
#endif
}

