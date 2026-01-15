/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font C:/Users/doubl/SquareLine/assets/16SegmentDisplay.ttf -o C:/Users/doubl/SquareLine/assets\ui_font_calc.c --format lvgl -r 0x20-0xff --symbols ね朝!上ピぃ暗洋ッ煙載留輸起2描4限絡花忘夫党働洗腕市賃ケ府御ガ転押種熱式緩多0雨菜絵過間士注二重千降郵亡デ障機ェ世信牛ゴ顔下9要安部側司ほ休ご歌定ぺ族咲酒則背よお初ペ反政妹伝元明校し災刊疲近績ニ響遠模電従株室肩象く線１シ企利能厳週づ薬曜遅辛房並脱使意湖グ思女酔火帳き以無ヒギ友算談類記冷拭悪リ呼逮壊ビス飛各う彼易業暖読必具契?寄も紙採寺葉少材号秘ゆ職説店守計7資港挙的百コい済ド独帰ュ柿簡万駄静令病「第入帯木和り冬求渡景支写長懸村塩終髪ゃ宅ン様果織荷昨マ夕仮急始テ歯認処3２案局日謝め伸へ経主台ぴ食他答離ヶタ影番天費声頃英財民態走光皆形宙涼高が録自犬貧ノ横告配味カ犯点違ネ派成銀最束通飲麗学ト米こぁぇ産渉画春修庫チ子沈兄交検ヤ菓方鳥選才買科だ8移製野ア欲換車策三管故立介真。律猿節都困ル売げ球温出死セ優航,商験特危今ツ勤午量土桃頼晴夏際補努西と緑L新全理目常魚坊界東八ろ候往余5打五失捨進忙聞行符返革密念授びク届先皿減表禁陽雲加隣不輪在小蔵格ダょパ息付改泣メ非予晩薄っな状ナ割オ基む切盗頭提公制毎金例宇弟取ち靴書嫌体州寒甘水型色当森でウす務探治性構射名キ得正来判ど路服化料そ文等砂座礼仕揺任暑歩触末ム続ザ冊普首中ラ直力習波ひ発ざモ残婚毛鼻ズの備辺価邪何参事販話教報代設退育内猫就願北イ遊エ漢申九ィー生強置悲ぐ締広断借短然閉や館込引白値ば結外現対幸田ん低次席町階O社昇億ゼ般知海震騒紀因辞島誌差図旅条ま奇幅らサ濃情鉛ジ消四誰列一レ徒笑規破肉机妻運円決腰ヨ雪六払太プ担風未由ャ分招訳待て根示バ昼札慣喉戻姉携越繰糖に屋査林音を勉石夜ボ履替祖怒乏」ぎ良法娘個青員曲母趣祭工国合杯浅停膝復足供空農投山動耳造着関貿貸船医調止痛祝じ秀6鉄組紹程茶住増手れ若質件準積建試平る月約心歳品ョ便気勝課役右負渇ぬ時酸命追練ァ議争複ゲ季寝ぞぽ流窓研功相映たか軽会地単左丈早久存黒七受字期送氏語楽泊神落害ぼ言可道古除ぶ汚宿、ベ責父ふ原感同係数況凍施詳筆比回深ミ人浴富前客去有秋用家後敗戦身男赤給せ接変緒半は泳解拡逃ず迫親術院星確卒嬉問駅評フ柱ゅ券ロ開胸究けさ川ポ指異わ橋口細者持ぜ活愛別更角似大僕乗向覚暇飯片狭十訪苦完考曇園周技連ハ難厚1ソ実諸協ぷ孫みぱ年えブ雑弱削池作凝べ速誕私美物京油氷々誘両達題ホ捕所面黄掛段好布営本討導つ位眠見南刻裏あ昔度械混ワ場集 --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_CALC
#define UI_FONT_CALC 1
#endif

#if UI_FONT_CALC

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x4, 0x92, 0x2, 0x49, 0x8,

    /* U+0022 "\"" */
    0x49, 0x99, 0x90,

    /* U+0023 "#" */
    0x3e, 0x24, 0x48, 0x91, 0x3f, 0x81, 0x12, 0x24,
    0x48, 0x90, 0x0,

    /* U+0024 "$" */
    0x3f, 0x48, 0x48, 0x48, 0x48, 0x7e, 0x2, 0x12,
    0x12, 0x12, 0x12, 0xfc,

    /* U+0025 "%" */
    0x72, 0x49, 0x66, 0x9b, 0xf2, 0x59, 0x66, 0x92,
    0x4e,

    /* U+0026 "&" */
    0x38, 0x11, 0x86, 0x19, 0xe8, 0xa2, 0x86, 0x18,
    0x3f,

    /* U+0027 "'" */
    0x6a, 0x80,

    /* U+0028 "(" */
    0x29, 0x41, 0x24, 0x40,

    /* U+0029 ")" */
    0x44, 0x90, 0x52, 0x80,

    /* U+002A "*" */
    0x9, 0x33, 0x4e, 0x3b, 0xb3, 0x1c, 0x6a, 0xa2,
    0x0,

    /* U+002B "+" */
    0x0, 0x41, 0x4, 0x13, 0xf0, 0x8, 0x20, 0x82,
    0x0,

    /* U+002C "," */
    0x25, 0x48,

    /* U+002D "-" */
    0xfc,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x4, 0x10, 0x84, 0x0, 0x84, 0x10, 0x80, 0x0,

    /* U+0030 "0" */
    0x1f, 0x41, 0x43, 0x45, 0x45, 0x8, 0x92, 0xa2,
    0xa2, 0xc2, 0x82, 0xfc,

    /* U+0031 "1" */
    0x15, 0x42, 0xa8,

    /* U+0032 "2" */
    0x1f, 0x1, 0x1, 0x1, 0x1, 0x7e, 0x80, 0x80,
    0x80, 0x80, 0x80, 0xfc,

    /* U+0033 "3" */
    0x1f, 0x1, 0x1, 0x1, 0x1, 0x7e, 0x2, 0x2,
    0x2, 0x2, 0x2, 0xfc,

    /* U+0034 "4" */
    0x43, 0x6, 0xc, 0x18, 0x3f, 0x81, 0x2, 0x4,
    0x8, 0x10, 0x0,

    /* U+0035 "5" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x2, 0x2,
    0x2, 0x2, 0x2, 0xfc,

    /* U+0036 "6" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+0037 "7" */
    0x7c, 0x10, 0x41, 0x4, 0x10, 0x2, 0x8, 0x20,
    0x82,

    /* U+0038 "8" */
    0x3f, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+0039 "9" */
    0x3f, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x2, 0x2,
    0x2, 0x2, 0x2, 0xfc,

    /* U+003A ":" */
    0x81,

    /* U+003B ";" */
    0x40, 0x0, 0x3, 0x6b, 0x0,

    /* U+003C "<" */
    0x1, 0x7, 0xc, 0x30, 0xc0, 0xc0, 0x30, 0xc,
    0x2,

    /* U+003D "=" */
    0x7e, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x0,

    /* U+003E ">" */
    0xf0, 0x7, 0x0, 0x38, 0x7, 0xf, 0x1c, 0xc,
    0x0,

    /* U+003F "?" */
    0x7f, 0x6, 0xc, 0x18, 0x23, 0x80, 0x10, 0x20,
    0x40, 0x80, 0x4, 0x0,

    /* U+0040 "@" */
    0x1f, 0x1, 0x1, 0x1, 0x1, 0x70, 0x82, 0x92,
    0x92, 0x92, 0x92, 0xfc,

    /* U+0041 "A" */
    0x3f, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x82, 0x82,
    0x82, 0x82, 0x82, 0x0,

    /* U+0042 "B" */
    0x1f, 0x9, 0x9, 0x9, 0x9, 0xe, 0x2, 0x12,
    0x12, 0x12, 0x12, 0xfc,

    /* U+0043 "C" */
    0x1f, 0x40, 0x40, 0x40, 0x40, 0x0, 0x80, 0x80,
    0x80, 0x80, 0x80, 0xfc,

    /* U+0044 "D" */
    0x1f, 0x9, 0x9, 0x9, 0x9, 0x0, 0x2, 0x12,
    0x12, 0x12, 0x12, 0xfc,

    /* U+0045 "E" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x80, 0x80,
    0x80, 0x80, 0x80, 0xfc,

    /* U+0046 "F" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x80, 0x80,
    0x80, 0x80, 0x80,

    /* U+0047 "G" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0xe, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+0048 "H" */
    0x21, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x82, 0x82,
    0x82, 0x82, 0x82, 0x0,

    /* U+0049 "I" */
    0x1f, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x10,
    0x10, 0x10, 0x10, 0xfc,

    /* U+004A "J" */
    0x0, 0x1, 0x1, 0x1, 0x1, 0x0, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+004B "K" */
    0x20, 0x21, 0x22, 0x22, 0x24, 0x38, 0x44, 0x44,
    0x44, 0x42, 0x80,

    /* U+004C "L" */
    0x21, 0x4, 0x10, 0x40, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+004D "M" */
    0x21, 0x63, 0x53, 0x55, 0x59, 0x0, 0x82, 0x82,
    0x82, 0x82, 0x82, 0x0,

    /* U+004E "N" */
    0x20, 0xb0, 0x94, 0x4a, 0x25, 0x10, 0x2, 0x25,
    0x14, 0x86, 0x43, 0x20, 0x80, 0x0,

    /* U+004F "O" */
    0x1f, 0x41, 0x41, 0x41, 0x41, 0x0, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+0050 "P" */
    0x3f, 0x41, 0x41, 0x41, 0x41, 0x7e, 0x80, 0x80,
    0x80, 0x80, 0x80,

    /* U+0051 "Q" */
    0x1f, 0x10, 0x48, 0x24, 0x12, 0x10, 0x1, 0x14,
    0x8a, 0x45, 0x21, 0xa0, 0x8f, 0xc0,

    /* U+0052 "R" */
    0x1f, 0x90, 0x48, 0x24, 0x12, 0x11, 0xf9, 0x10,
    0x88, 0x44, 0x21, 0x20, 0x0,

    /* U+0053 "S" */
    0x3f, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x2, 0x2,
    0x2, 0x2, 0x2, 0xfc,

    /* U+0054 "T" */
    0x7c, 0x82, 0x8, 0x20, 0x0, 0x10, 0x41, 0x4,
    0x10,

    /* U+0055 "U" */
    0x20, 0x41, 0x41, 0x41, 0x41, 0x0, 0x82, 0x82,
    0x82, 0x82, 0x82, 0xfc,

    /* U+0056 "V" */
    0x20, 0x85, 0xa, 0x24, 0x80, 0x24, 0x50, 0xa1,
    0x82, 0x0,

    /* U+0057 "W" */
    0x20, 0x90, 0x48, 0x24, 0x12, 0x10, 0x1, 0x54,
    0xaa, 0x65, 0x31, 0x90, 0x80, 0x0,

    /* U+0058 "X" */
    0x44, 0x92, 0x8a, 0x0, 0xc5, 0x12, 0x88, 0x0,

    /* U+0059 "Y" */
    0x43, 0x6, 0xc, 0x18, 0x3f, 0x80, 0x10, 0x20,
    0x40, 0x80,

    /* U+005A "Z" */
    0x1f, 0x0, 0x2, 0x4, 0x4, 0x8, 0x10, 0x20,
    0x20, 0x40, 0x0, 0xfc,

    /* U+005B "[" */
    0x1a, 0x10, 0x84, 0x2, 0x10, 0x84, 0x21, 0xc0,

    /* U+005C "\\" */
    0x84, 0x44, 0x2, 0x21, 0x10,

    /* U+005D "]" */
    0x18, 0x42, 0x10, 0x80, 0x2, 0x10, 0x85, 0xc0,

    /* U+005E "^" */
    0x5a, 0xda,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x95,

    /* U+0061 "a" */
    0x72, 0x9, 0x24, 0x92, 0x4f, 0xc0,

    /* U+0062 "b" */
    0x24, 0x44, 0x47, 0x89, 0x99, 0x9e,

    /* U+0063 "c" */
    0x7f, 0x2, 0x4, 0x8, 0x10, 0x3f, 0x0,

    /* U+0064 "d" */
    0x0, 0x42, 0x10, 0xba, 0x12, 0x94, 0xa5, 0xc0,

    /* U+0065 "e" */
    0x72, 0x4a, 0x28, 0xc2, 0xf, 0xc0,

    /* U+0066 "f" */
    0xe, 0x20, 0x40, 0x81, 0x1f, 0x80, 0x10, 0x20,
    0x40, 0x80,

    /* U+0067 "g" */
    0x3a, 0x52, 0x94, 0xb8, 0x2, 0x10, 0x85, 0xc0,

    /* U+0068 "h" */
    0x24, 0x44, 0x47, 0x89, 0x99, 0x90,

    /* U+0069 "i" */
    0x40, 0x2a, 0x80,

    /* U+006A "j" */
    0x2, 0x0, 0x0, 0x0, 0x40, 0x81, 0x4, 0x0,
    0x1, 0x22, 0x44, 0x92, 0x24, 0x30,

    /* U+006B "k" */
    0x21, 0x4c, 0x62, 0x1, 0x8c, 0x62, 0xa0,

    /* U+006C "l" */
    0x9, 0x24, 0x4, 0x92, 0x70,

    /* U+006D "m" */
    0x7f, 0x6, 0x4c, 0x99, 0x32, 0x40, 0x0,

    /* U+006E "n" */
    0x7f, 0x6, 0xc, 0x18, 0x30, 0x40, 0x0,

    /* U+006F "o" */
    0x7f, 0x6, 0xc, 0x18, 0x30, 0x7f, 0x0,

    /* U+0070 "p" */
    0x3a, 0x52, 0x94, 0xba, 0x10, 0x84, 0x20,

    /* U+0071 "q" */
    0x74, 0xa5, 0x29, 0x70, 0x4, 0x21, 0x8, 0x70,

    /* U+0072 "r" */
    0x7f, 0x2, 0x4, 0x8, 0x10, 0x0,

    /* U+0073 "s" */
    0x74, 0x44, 0x20, 0xe0,

    /* U+0074 "t" */
    0x0, 0x41, 0x4, 0x13, 0xf0, 0x8, 0x20, 0x82,
    0xe,

    /* U+0075 "u" */
    0x82, 0x49, 0x24, 0x93, 0xf0,

    /* U+0076 "v" */
    0x9a, 0xac, 0x80,

    /* U+0077 "w" */
    0x83, 0x26, 0x4c, 0x99, 0x3f, 0x80,

    /* U+0078 "x" */
    0x7e, 0x0, 0x40, 0x81, 0x2, 0x3f, 0x0,

    /* U+0079 "y" */
    0x22, 0x52, 0x94, 0xb8, 0x2, 0x10, 0x85, 0xc0,

    /* U+007A "z" */
    0x71, 0x22, 0x40, 0xe0,

    /* U+007B "{" */
    0xe, 0x20, 0x40, 0x81, 0x1c, 0x0, 0x10, 0x20,
    0x40, 0x81, 0xc0,

    /* U+007C "|" */
    0x15, 0x42, 0xa8,

    /* U+007D "}" */
    0x18, 0x10, 0x20, 0x40, 0x81, 0xc0, 0x8, 0x10,
    0x20, 0x47, 0x0,

    /* U+007E "~" */
    0x43, 0x1a, 0x6a, 0xb0,

    /* U+00A1 "¡" */
    0xa2, 0xab, 0x54,

    /* U+00A2 "¢" */
    0x10, 0x20, 0x43, 0xec, 0x30, 0x20, 0x40, 0x82,
    0xc8, 0xe0, 0x81, 0x0,

    /* U+00A3 "£" */
    0x1c, 0x32, 0x22, 0x20, 0x20, 0xfc, 0x20, 0x20,
    0x20, 0x20, 0x40, 0xfe,

    /* U+00A4 "¤" */
    0x83, 0xfd, 0x14, 0x18, 0x30, 0xdb, 0x5d,

    /* U+00A5 "¥" */
    0x44, 0x92, 0x8c, 0xfc, 0x2, 0x8, 0x20, 0x80,

    /* U+00A6 "¦" */
    0x4, 0x92, 0x0, 0x49, 0x20,

    /* U+00A7 "§" */
    0xe, 0x12, 0x21, 0x20, 0x20, 0x30, 0x58, 0xc6,
    0x82, 0x41, 0x33, 0x1c, 0x2, 0x3, 0x22, 0x36,
    0x18,

    /* U+00A8 "¨" */
    0x99, 0x90,

    /* U+00A9 "©" */
    0x0, 0x0, 0xf0, 0x31, 0xc4, 0xc, 0x4e, 0x69,
    0x36, 0x91, 0x29, 0x2, 0x91, 0x69, 0xb6, 0x4c,
    0x42, 0xc, 0x1f, 0x0,

    /* U+00AA "ª" */
    0x31, 0x20, 0x9e, 0xd9, 0xef, 0xf8,

    /* U+00AB "«" */
    0x27, 0x9b, 0x63, 0x60, 0x20,

    /* U+00AC "¬" */
    0xff, 0x1, 0x1, 0x1, 0x1,

    /* U+00AE "®" */
    0xf, 0x83, 0x8c, 0x7e, 0x6d, 0x12, 0x93, 0x29,
    0xc2, 0x93, 0x19, 0x13, 0xc9, 0x24, 0x84, 0x30,
    0x81, 0xf0,

    /* U+00AF "¯" */
    0x7e,

    /* U+00B0 "°" */
    0x31, 0x3c, 0x51, 0x7c, 0x60,

    /* U+00B1 "±" */
    0x4, 0x2, 0x1f, 0xe0, 0x80, 0x40, 0x3, 0xfe,

    /* U+00B2 "²" */
    0x64, 0xa4, 0x44, 0x43, 0xe0,

    /* U+00B3 "³" */
    0xf8, 0x43, 0x12, 0xa, 0x2f, 0x80,

    /* U+00B4 "´" */
    0x2f, 0x40,

    /* U+00B5 "µ" */
    0x10, 0x8, 0x88, 0x44, 0x26, 0x22, 0x11, 0x19,
    0x8c, 0xcb, 0xd8, 0x20, 0x10, 0x0,

    /* U+00B6 "¶" */
    0x3f, 0x25, 0x22, 0x91, 0x48, 0xa4, 0x5b, 0xf8,
    0x14, 0xa, 0x5, 0x2, 0x81, 0x40, 0xa0, 0x50,
    0x8,

    /* U+00B7 "·" */
    0x70,

    /* U+00B8 "¸" */
    0x4e, 0x16,

    /* U+00B9 "¹" */
    0x1, 0xcb, 0x8, 0x20, 0x82, 0x3f,

    /* U+00BA "º" */
    0x11, 0x38, 0x71, 0x78, 0x4f, 0xc0,

    /* U+00BB "»" */
    0x59, 0xb3, 0x5b, 0x58, 0x40,

    /* U+00BC "¼" */
    0x20, 0x3, 0x2, 0x28, 0x30, 0x41, 0x2, 0x10,
    0x11, 0x80, 0x89, 0x9e, 0x88, 0xc, 0x80, 0x49,
    0x4, 0xfe, 0x20, 0x42, 0x2, 0x0, 0x0,

    /* U+00BD "½" */
    0x20, 0x86, 0x18, 0xa1, 0x2, 0x30, 0x22, 0x2,
    0x6f, 0xf5, 0x90, 0x81, 0x8, 0x21, 0x4, 0x10,
    0xc2, 0x1f, 0x60, 0x4, 0x0,

    /* U+00BE "¾" */
    0x0, 0x3, 0xc1, 0x6, 0x8, 0x20, 0x80, 0x84,
    0x4, 0x4a, 0x62, 0xce, 0x24, 0x3, 0x50, 0x14,
    0x81, 0x3e, 0x8, 0x20, 0x81, 0x4, 0x0, 0x40,
    0x0,

    /* U+00BF "¿" */
    0x20, 0x1, 0x4, 0x11, 0xc4, 0x20, 0x86, 0x1f,
    0x88,

    /* U+00D0 "Ð" */
    0x3f, 0x84, 0x18, 0x81, 0x90, 0x12, 0x2, 0x40,
    0x7f, 0x9, 0x1, 0x20, 0x24, 0xc, 0x81, 0x10,
    0xc3, 0xf0,

    /* U+00D7 "×" */
    0x63, 0x36, 0x18, 0x1c, 0x24, 0x42, 0x1,

    /* U+00DE "Þ" */
    0x80, 0x40, 0x3f, 0x90, 0x68, 0x1c, 0x6, 0x5,
    0xe, 0xfc, 0x40, 0x20, 0x10, 0x0, 0x0,

    /* U+00DF "ß" */
    0x1, 0xf2, 0x34, 0x28, 0x50, 0xa6, 0x42, 0x85,
    0x6, 0xc, 0x19, 0xc1, 0x0,

    /* U+00F0 "ð" */
    0x0, 0x71, 0xc2, 0xc0, 0x87, 0x93, 0x63, 0x83,
    0x6, 0xc, 0x14, 0x47, 0x0,

    /* U+00F7 "÷" */
    0x0, 0x4, 0x0, 0x0, 0xf, 0xf8, 0x0, 0x0,
    0x10, 0xc, 0x0,

    /* U+00FE "þ" */
    0x81, 0x2, 0x5, 0xce, 0x58, 0x70, 0xe1, 0xc3,
    0x8b, 0x95, 0xc8, 0x10, 0x20, 0x40
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 77, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 122, .box_w = 3, .box_h = 13, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 122, .box_w = 4, .box_h = 5, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 9, .adv_w = 168, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 20, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 32, .adv_w = 153, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 41, .adv_w = 131, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 50, .adv_w = 76, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 52, .adv_w = 84, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 56, .adv_w = 99, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 60, .adv_w = 151, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 69, .adv_w = 145, .box_w = 6, .box_h = 11, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 78, .adv_w = 84, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 80, .adv_w = 145, .box_w = 6, .box_h = 1, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 81, .adv_w = 62, .box_w = 1, .box_h = 1, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 151, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 90, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 102, .adv_w = 122, .box_w = 2, .box_h = 11, .ofs_x = 4, .ofs_y = 2},
    {.bitmap_index = 105, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 117, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 129, .adv_w = 168, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 140, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 152, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 164, .adv_w = 168, .box_w = 6, .box_h = 12, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 173, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 185, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 197, .adv_w = 71, .box_w = 1, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 75, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 203, .adv_w = 162, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 219, .adv_w = 172, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 228, .adv_w = 168, .box_w = 7, .box_h = 13, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 252, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 264, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 276, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 288, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 300, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 312, .adv_w = 161, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 323, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 335, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 347, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 359, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 371, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 382, .adv_w = 131, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 391, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 403, .adv_w = 168, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 417, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 429, .adv_w = 168, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 440, .adv_w = 168, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 454, .adv_w = 168, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 467, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 479, .adv_w = 161, .box_w = 6, .box_h = 12, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 488, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 500, .adv_w = 151, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 510, .adv_w = 168, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 524, .adv_w = 151, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 532, .adv_w = 168, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 542, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 554, .adv_w = 114, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 562, .adv_w = 128, .box_w = 4, .box_h = 10, .ofs_x = 3, .ofs_y = 2},
    {.bitmap_index = 567, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 575, .adv_w = 99, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 577, .adv_w = 131, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 578, .adv_w = 99, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 579, .adv_w = 131, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 585, .adv_w = 103, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 591, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 598, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 606, .adv_w = 131, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 612, .adv_w = 161, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 622, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 630, .adv_w = 103, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 636, .adv_w = 118, .box_w = 2, .box_h = 10, .ofs_x = 4, .ofs_y = 1},
    {.bitmap_index = 639, .adv_w = 127, .box_w = 7, .box_h = 16, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 653, .adv_w = 151, .box_w = 5, .box_h = 11, .ofs_x = 3, .ofs_y = 2},
    {.bitmap_index = 660, .adv_w = 131, .box_w = 3, .box_h = 12, .ofs_x = 4, .ofs_y = 1},
    {.bitmap_index = 665, .adv_w = 153, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 672, .adv_w = 153, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 679, .adv_w = 153, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 686, .adv_w = 122, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 693, .adv_w = 131, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 701, .adv_w = 145, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 707, .adv_w = 90, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 711, .adv_w = 145, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 720, .adv_w = 131, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 725, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 728, .adv_w = 153, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 734, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 741, .adv_w = 122, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 749, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 753, .adv_w = 161, .box_w = 7, .box_h = 12, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 764, .adv_w = 122, .box_w = 2, .box_h = 11, .ofs_x = 4, .ofs_y = 2},
    {.bitmap_index = 767, .adv_w = 145, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 778, .adv_w = 151, .box_w = 6, .box_h = 5, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 782, .adv_w = 80, .box_w = 2, .box_h = 11, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 785, .adv_w = 146, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 797, .adv_w = 151, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 809, .adv_w = 147, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 816, .adv_w = 151, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 824, .adv_w = 122, .box_w = 3, .box_h = 13, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 141, .box_w = 8, .box_h = 17, .ofs_x = -1, .ofs_y = -4},
    {.bitmap_index = 846, .adv_w = 91, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 848, .adv_w = 197, .box_w = 12, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 868, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 874, .adv_w = 146, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 879, .adv_w = 159, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 884, .adv_w = 204, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 902, .adv_w = 108, .box_w = 7, .box_h = 1, .ofs_x = -1, .ofs_y = 10},
    {.bitmap_index = 903, .adv_w = 115, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 908, .adv_w = 163, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 916, .adv_w = 111, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 921, .adv_w = 111, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 927, .adv_w = 87, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 929, .adv_w = 147, .box_w = 9, .box_h = 12, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 943, .adv_w = 164, .box_w = 9, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 960, .adv_w = 69, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 961, .adv_w = 95, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 963, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 969, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 975, .adv_w = 132, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 980, .adv_w = 229, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1003, .adv_w = 221, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1024, .adv_w = 231, .box_w = 13, .box_h = 15, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1049, .adv_w = 143, .box_w = 6, .box_h = 12, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 1058, .adv_w = 193, .box_w = 11, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1076, .adv_w = 148, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1083, .adv_w = 175, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1098, .adv_w = 159, .box_w = 7, .box_h = 14, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 1111, .adv_w = 151, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1124, .adv_w = 156, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1135, .adv_w = 149, .box_w = 7, .box_h = 16, .ofs_x = 1, .ofs_y = -4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_3[] = {
    0x0, 0x7, 0xe, 0xf, 0x20, 0x27, 0x2e
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 161, .range_length = 12, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 174, .range_length = 18, .glyph_id_start = 108,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 208, .range_length = 47, .glyph_id_start = 126,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 7, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 4,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_calc = {
#else
lv_font_t ui_font_calc = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_CALC*/

