/* C++ code produced by gperf version 3.0.1 */
/* Command-line: c:/utils/gperf.exe -t -L C++ -H htmlentities -N find_def -Z html_entities -D html_entities.txt  */
/* Computed positions: -k'1-3,5,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "html_entities.txt"
struct html_entity_def { const char *name; wchar value; };

#define TOTAL_KEYWORDS 256
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 720
/* maximum key range = 713, duplicates = 0 */

class html_entities
{
private:
  static inline unsigned int htmlentities (const char *str, unsigned int len);
public:
  static struct html_entity_def *find_def (const char *str, unsigned int len);
};

inline unsigned int
html_entities::htmlentities (register const char *str, register unsigned int len)
{
  static unsigned short asso_values[] =
    {
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721,  20,
       30,  35,   0, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 250, 210,  30,  20, 125,
        5,   0,   0, 305, 721,  15,   5,   5,  15, 155,
      110, 721,   5,  35,  60, 100, 721, 721,   0,  20,
        0, 721, 721, 721, 721, 721, 721,   5,  60,  50,
        0,  15, 275,  35, 120,  10, 180,  10,  95, 125,
       25,   0,   5, 390,  90,  20,   0,  65,  35,   0,
       30, 165, 165,   0, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721, 721, 721, 721,
      721, 721, 721, 721, 721, 721, 721
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]+1];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct html_entity_def *
html_entities::find_def (register const char *str, register unsigned int len)
{
  static struct html_entity_def wordlist[] =
    {
#line 70 "html_entities.txt"
      {"and", 8743},
#line 133 "html_entities.txt"
      {"int", 8747},
#line 47 "html_entities.txt"
      {"Rho", 929},
#line 134 "html_entities.txt"
      {"iota", 953},
#line 198 "html_entities.txt"
      {"psi", 968},
#line 196 "html_entities.txt"
      {"prod", 8719},
#line 165 "html_entities.txt"
      {"not", 172},
#line 197 "html_entities.txt"
      {"prop", 8733},
#line 189 "html_entities.txt"
      {"phi", 966},
#line 217 "html_entities.txt"
      {"sdot", 8901},
#line 235 "html_entities.txt"
      {"theta", 952},
#line 69 "html_entities.txt"
      {"amp", 38},
#line 104 "html_entities.txt"
      {"ensp", 8194},
#line 234 "html_entities.txt"
      {"there4", 8756},
#line 136 "html_entities.txt"
      {"isin", 8712},
#line 237 "html_entities.txt"
      {"thinsp", 8201},
#line 175 "html_entities.txt"
      {"omega", 969},
#line 216 "html_entities.txt"
      {"scaron", 353},
#line 241 "html_entities.txt"
      {"trade", 8482},
#line 13 "html_entities.txt"
      {"Chi", 935},
#line 190 "html_entities.txt"
      {"phiv", 981},
#line 238 "html_entities.txt"
      {"thorn", 254},
#line 227 "html_entities.txt"
      {"sup", 8835},
#line 103 "html_entities.txt"
      {"emsp", 8195},
#line 195 "html_entities.txt"
      {"prime", 8242},
#line 48 "html_entities.txt"
      {"Scaron", 352},
#line 131 "html_entities.txt"
      {"image", 8465},
#line 71 "html_entities.txt"
      {"ang", 8736},
#line 231 "html_entities.txt"
      {"supe", 8839},
#line 194 "html_entities.txt"
      {"pound", 163},
#line 84 "html_entities.txt"
      {"chi", 967},
#line 228 "html_entities.txt"
      {"sup1", 185},
#line 166 "html_entities.txt"
      {"notin", 8713},
#line 138 "html_entities.txt"
      {"kappa", 954},
#line 108 "html_entities.txt"
      {"eta", 951},
#line 229 "html_entities.txt"
      {"sup2", 178},
#line 29 "html_entities.txt"
      {"Kappa", 922},
#line 182 "html_entities.txt"
      {"otilde", 245},
#line 90 "html_entities.txt"
      {"cup", 8746},
#line 230 "html_entities.txt"
      {"sup3", 179},
#line 52 "html_entities.txt"
      {"Theta", 920},
#line 74 "html_entities.txt"
      {"atilde", 227},
#line 121 "html_entities.txt"
      {"gt", 62},
#line 211 "html_entities.txt"
      {"rho", 961},
#line 161 "html_entities.txt"
      {"nbsp", 160},
#line 64 "html_entities.txt"
      {"acute", 180},
#line 31 "html_entities.txt"
      {"Mu", 924},
#line 72 "html_entities.txt"
      {"aring", 229},
#line 32 "html_entities.txt"
      {"Ntilde", 209},
#line 178 "html_entities.txt"
      {"or", 8744},
#line 171 "html_entities.txt"
      {"ocirc", 244},
#line 183 "html_entities.txt"
      {"otimes", 8855},
#line 33 "html_entities.txt"
      {"Nu", 925},
#line 87 "html_entities.txt"
      {"cong", 8773},
#line 63 "html_entities.txt"
      {"acirc", 226},
#line 168 "html_entities.txt"
      {"ntilde", 241},
#line 80 "html_entities.txt"
      {"cap", 8745},
#line 128 "html_entities.txt"
      {"icirc", 238},
#line 148 "html_entities.txt"
      {"lowast", 8727},
#line 169 "html_entities.txt"
      {"nu", 957},
#line 100 "html_entities.txt"
      {"ecirc", 234},
#line 170 "html_entities.txt"
      {"oacute", 243},
#line 46 "html_entities.txt"
      {"Psi", 936},
#line 225 "html_entities.txt"
      {"sube", 8838},
#line 62 "html_entities.txt"
      {"aacute", 225},
#line 127 "html_entities.txt"
      {"iacute", 237},
#line 43 "html_entities.txt"
      {"Phi", 934},
#line 111 "html_entities.txt"
      {"euro", 8364},
#line 99 "html_entities.txt"
      {"eacute", 233},
#line 180 "html_entities.txt"
      {"ordm", 186},
#line 68 "html_entities.txt"
      {"alpha", 945},
#line 59 "html_entities.txt"
      {"Yacute", 221},
#line 167 "html_entities.txt"
      {"nsub", 8836},
#line 12 "html_entities.txt"
      {"Ccedil", 199},
#line 176 "html_entities.txt"
      {"omicron", 959},
#line 186 "html_entities.txt"
      {"part", 8706},
#line 160 "html_entities.txt"
      {"nabla", 8711},
#line 153 "html_entities.txt"
      {"lt", 60},
#line 236 "html_entities.txt"
      {"thetasym", 977},
#line 185 "html_entities.txt"
      {"para", 182},
#line 213 "html_entities.txt"
      {"rsaquo", 8250},
#line 174 "html_entities.txt"
      {"oline", 8254},
#line 151 "html_entities.txt"
      {"lsaquo", 8249},
#line 45 "html_entities.txt"
      {"Prime", 8243},
#line 81 "html_entities.txt"
      {"ccedil", 231},
#line 224 "html_entities.txt"
      {"sub", 8834},
#line 245 "html_entities.txt"
      {"ucirc", 251},
#line 73 "html_entities.txt"
      {"asymp", 8776},
#line 232 "html_entities.txt"
      {"szlig", 223},
#line 135 "html_entities.txt"
      {"iquest", 191},
#line 58 "html_entities.txt"
      {"Xi", 926},
#line 233 "html_entities.txt"
      {"tau", 964},
#line 112 "html_entities.txt"
      {"exist", 8707},
#line 243 "html_entities.txt"
      {"uacute", 250},
#line 191 "html_entities.txt"
      {"pi", 960},
#line 23 "html_entities.txt"
      {"Gamma", 915},
#line 30 "html_entities.txt"
      {"Lambda", 923},
#line 21 "html_entities.txt"
      {"Eta", 919},
#line 93 "html_entities.txt"
      {"dagger", 8224},
#line 38 "html_entities.txt"
      {"Omega", 937},
#line 203 "html_entities.txt"
      {"rang", 9002},
#line 54 "html_entities.txt"
      {"Ucirc", 219},
#line 78 "html_entities.txt"
      {"brvbar", 166},
#line 164 "html_entities.txt"
      {"ni", 8715},
#line 141 "html_entities.txt"
      {"lang", 9001},
#line 91 "html_entities.txt"
      {"curren", 164},
#line 252 "html_entities.txt"
      {"xi", 958},
#line 14 "html_entities.txt"
      {"Dagger", 8225},
#line 159 "html_entities.txt"
      {"mu", 956},
#line 16 "html_entities.txt"
      {"ETH", 208,	    },
#line 88 "html_entities.txt"
      {"copy", 169},
#line 97 "html_entities.txt"
      {"diams", 9830},
#line 53 "html_entities.txt"
      {"Uacute", 218},
#line 119 "html_entities.txt"
      {"gamma", 947},
#line 98 "html_entities.txt"
      {"divide", 247},
#line 18 "html_entities.txt"
      {"Ecirc", 202},
#line 181 "html_entities.txt"
      {"oslash", 248},
#line 94 "html_entities.txt"
      {"darr", 8595},
#line 107 "html_entities.txt"
      {"equiv", 8801},
#line 220 "html_entities.txt"
      {"sigma", 963},
#line 41 "html_entities.txt"
      {"Otilde", 213},
#line 51 "html_entities.txt"
      {"Tau", 932},
#line 202 "html_entities.txt"
      {"radic", 8730},
#line 17 "html_entities.txt"
      {"Eacute", 201},
#line 192 "html_entities.txt"
      {"piv", 982},
#line 184 "html_entities.txt"
      {"ouml", 246},
#line 89 "html_entities.txt"
      {"crarr", 8629},
#line 75 "html_entities.txt"
      {"auml", 228},
#line 49 "html_entities.txt"
      {"Sigma", 931},
#line 173 "html_entities.txt"
      {"ograve", 242},
#line 137 "html_entities.txt"
      {"iuml", 239},
#line 36 "html_entities.txt"
      {"Ocirc", 212},
#line 66 "html_entities.txt"
      {"agrave", 224},
#line 110 "html_entities.txt"
      {"euml", 235},
#line 130 "html_entities.txt"
      {"igrave", 236},
#line 60 "html_entities.txt"
      {"Yuml", 376},
#line 101 "html_entities.txt"
      {"egrave", 232},
#line 247 "html_entities.txt"
      {"uml", 168},
#line 61 "html_entities.txt"
      {"Zeta", 918},
#line 86 "html_entities.txt"
      {"clubs", 9827},
#line 35 "html_entities.txt"
      {"Oacute", 211},
#line 79 "html_entities.txt"
      {"bull", 8226},
#line 162 "html_entities.txt"
      {"ndash", 8211},
#line 140 "html_entities.txt"
      {"lambda", 955},
#line 253 "html_entities.txt"
      {"yacute", 253},
#line 67 "html_entities.txt"
      {"alefsym", 8501},
#line 179 "html_entities.txt"
      {"ordf", 170},
#line 206 "html_entities.txt"
      {"rceil", 8969},
#line 44 "html_entities.txt"
      {"Pi", 928},
#line 144 "html_entities.txt"
      {"lceil", 8968},
#line 226 "html_entities.txt"
      {"sum", 8721},
#line 244 "html_entities.txt"
      {"uarr", 8593},
#line 239 "html_entities.txt"
      {"tilde", 732},
#line 157 "html_entities.txt"
      {"middot", 183},
#line 39 "html_entities.txt"
      {"Omicron", 927},
#line 210 "html_entities.txt"
      {"rfloor", 8971},
#line 163 "html_entities.txt"
      {"ne", 8800},
#line 27 "html_entities.txt"
      {"Iota", 921},
#line 147 "html_entities.txt"
      {"lfloor", 8970},
#line 109 "html_entities.txt"
      {"eth", 240},
#line 250 "html_entities.txt"
      {"uuml", 252},
#line 116 "html_entities.txt"
      {"frac14", 188},
#line 120 "html_entities.txt"
      {"ge", 8805},
#line 154 "html_entities.txt"
      {"macr", 175},
#line 34 "html_entities.txt"
      {"OElig", 338},
#line 246 "html_entities.txt"
      {"ugrave", 249},
#line 205 "html_entities.txt"
      {"rarr", 8594},
#line 126 "html_entities.txt"
      {"hyphen", 173},
#line 143 "html_entities.txt"
      {"larr", 8592},
#line 132 "html_entities.txt"
      {"infin", 8734},
#line 117 "html_entities.txt"
      {"frac34", 190},
#line 77 "html_entities.txt"
      {"beta", 946},
#line 9 "html_entities.txt"
      {"Atilde", 195},
#line 95 "html_entities.txt"
      {"deg", 176},
#line 218 "html_entities.txt"
      {"sect", 167},
#line 240 "html_entities.txt"
      {"times", 215},
#line 193 "html_entities.txt"
      {"plusmn", 177},
#line 83 "html_entities.txt"
      {"cent", 162},
#line 8 "html_entities.txt"
      {"Aring", 197},
#line 115 "html_entities.txt"
      {"frac12", 189},
#line 57 "html_entities.txt"
      {"Uuml", 220},
#line 156 "html_entities.txt"
      {"micro", 181},
#line 219 "html_entities.txt"
      {"shy", 173},
#line 123 "html_entities.txt"
      {"harr", 8596},
#line 5 "html_entities.txt"
      {"Acirc", 194},
#line 55 "html_entities.txt"
      {"Ugrave", 217},
#line 150 "html_entities.txt"
      {"lrm", 8206},
#line 85 "html_entities.txt"
      {"circ", 710},
#line 158 "html_entities.txt"
      {"minus", 8722},
#line 188 "html_entities.txt"
      {"perp", 8869},
#line 102 "html_entities.txt"
      {"empty", 8709},
#line 4 "html_entities.txt"
      {"Aacute", 193},
#line 22 "html_entities.txt"
      {"Euml", 203},
#line 96 "html_entities.txt"
      {"delta", 948},
#line 251 "html_entities.txt"
      {"weierp", 8472},
#line 146 "html_entities.txt"
      {"le", 8804},
#line 155 "html_entities.txt"
      {"mdash", 8212},
#line 19 "html_entities.txt"
      {"Egrave", 200},
#line 92 "html_entities.txt"
      {"dArr", 8659},
#line 7 "html_entities.txt"
      {"Alpha", 913},
#line 40 "html_entities.txt"
      {"Oslash", 216},
#line 258 "html_entities.txt"
      {"zwnj", 8204},
#line 15 "html_entities.txt"
      {"Delta", 916},
#line 42 "html_entities.txt"
      {"Ouml", 214},
#line 200 "html_entities.txt"
      {"apos", '\''},
#line 25 "html_entities.txt"
      {"Icirc", 206},
#line 37 "html_entities.txt"
      {"Ograve", 210},
#line 255 "html_entities.txt"
      {"yuml", 255},
#line 3 "html_entities.txt"
      {"AElig", 198},
#line 124 "html_entities.txt"
      {"hearts", 9829},
#line 199 "html_entities.txt"
      {"quot", 34},
#line 149 "html_entities.txt"
      {"loz", 9674},
#line 24 "html_entities.txt"
      {"Iacute", 205},
#line 209 "html_entities.txt"
      {"reg", 174},
#line 105 "html_entities.txt"
      {"epsi", 949},
#line 172 "html_entities.txt"
      {"oelig", 339},
#line 256 "html_entities.txt"
      {"zeta", 950},
#line 65 "html_entities.txt"
      {"aelig", 230},
#line 222 "html_entities.txt"
      {"sim", 8764},
#line 223 "html_entities.txt"
      {"spades", 9824},
#line 242 "html_entities.txt"
      {"uArr", 8657},
#line 215 "html_entities.txt"
      {"sbquo", 8218},
#line 212 "html_entities.txt"
      {"rlm", 8207},
#line 208 "html_entities.txt"
      {"real", 8476},
#line 76 "html_entities.txt"
      {"bdquo", 8222,	},
#line 187 "html_entities.txt"
      {"permil", 8240},
#line 201 "html_entities.txt"
      {"rArr", 8658},
#line 214 "html_entities.txt"
      {"rsquo", 8217},
#line 139 "html_entities.txt"
      {"lArr", 8656},
#line 152 "html_entities.txt"
      {"lsquo", 8216},
#line 254 "html_entities.txt"
      {"yen", 165},
#line 11 "html_entities.txt"
      {"Beta", 914},
#line 118 "html_entities.txt"
      {"frasl", 8260},
#line 207 "html_entities.txt"
      {"rdquo", 8221},
#line 145 "html_entities.txt"
      {"ldquo", 8220},
#line 10 "html_entities.txt"
      {"Auml", 196},
#line 129 "html_entities.txt"
      {"iexcl", 161},
#line 125 "html_entities.txt"
      {"hellip", 8230},
#line 122 "html_entities.txt"
      {"hArr", 8660},
#line 6 "html_entities.txt"
      {"Agrave", 192},
#line 82 "html_entities.txt"
      {"cedil", 184},
#line 221 "html_entities.txt"
      {"sigmaf", 962},
#line 177 "html_entities.txt"
      {"oplus", 8853},
#line 204 "html_entities.txt"
      {"raquo", 187},
#line 142 "html_entities.txt"
      {"laquo", 171},
#line 106 "html_entities.txt"
      {"epsilon", 949},
#line 113 "html_entities.txt"
      {"fnof", 402},
#line 50 "html_entities.txt"
      {"THORN", 222},
#line 257 "html_entities.txt"
      {"zwj", 8205},
#line 28 "html_entities.txt"
      {"Iuml", 207},
#line 114 "html_entities.txt"
      {"forall", 8704},
#line 26 "html_entities.txt"
      {"Igrave", 204},
#line 249 "html_entities.txt"
      {"upsilon", 965},
#line 56 "html_entities.txt"
      {"Upsilon", 933},
#line 20 "html_entities.txt"
      {"Epsilon", 917},
#line 248 "html_entities.txt"
      {"upsih", 978}
    };

  static short lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,  -1,
       -1,  -1,  -1,   1,  -1,  -1,  -1,  -1,   2,  -1,
       -1,  -1,  -1,  -1,   3,  -1,  -1,  -1,   4,   5,
       -1,  -1,  -1,   6,   7,  -1,  -1,  -1,   8,   9,
       10,  -1,  -1,  11,  12,  -1,  13,  -1,  -1,  14,
       -1,  15,  -1,  -1,  -1,  16,  17,  -1,  -1,  -1,
       18,  -1,  -1,  19,  20,  21,  -1,  -1,  22,  23,
       24,  25,  -1,  -1,  -1,  26,  -1,  -1,  27,  28,
       29,  -1,  -1,  30,  31,  32,  -1,  -1,  -1,  -1,
       33,  -1,  -1,  34,  35,  36,  37,  -1,  38,  39,
       40,  41,  42,  43,  44,  45,  -1,  46,  -1,  -1,
       47,  48,  49,  -1,  -1,  50,  51,  52,  -1,  53,
       54,  55,  -1,  56,  -1,  57,  58,  59,  -1,  -1,
       60,  61,  -1,  62,  63,  -1,  64,  -1,  -1,  -1,
       -1,  65,  -1,  66,  67,  -1,  68,  -1,  -1,  69,
       70,  71,  -1,  -1,  72,  -1,  73,  74,  -1,  75,
       76,  -1,  77,  78,  79,  -1,  80,  -1,  -1,  -1,
       81,  82,  -1,  -1,  -1,  83,  84,  -1,  85,  -1,
       86,  -1,  -1,  -1,  -1,  87,  -1,  -1,  -1,  -1,
       88,  89,  90,  91,  -1,  92,  93,  94,  -1,  -1,
       95,  96,  -1,  97,  -1,  -1,  98,  -1,  -1,  -1,
       99,  -1,  -1,  -1, 100, 101, 102, 103,  -1, 104,
       -1, 105, 106,  -1,  -1,  -1, 107, 108, 109, 110,
      111, 112,  -1,  -1,  -1, 113, 114,  -1,  -1,  -1,
      115, 116,  -1,  -1, 117, 118,  -1,  -1,  -1,  -1,
      119, 120,  -1, 121,  -1, 122, 123,  -1, 124, 125,
      126,  -1,  -1,  -1, 127, 128, 129,  -1,  -1, 130,
      131, 132,  -1,  -1, 133,  -1, 134,  -1,  -1, 135,
       -1, 136,  -1, 137, 138, 139, 140,  -1,  -1, 141,
      142, 143,  -1,  -1,  -1,  -1, 144, 145,  -1, 146,
      147,  -1, 148,  -1,  -1, 149,  -1,  -1, 150, 151,
      152, 153, 154,  -1,  -1,  -1, 155, 156,  -1, 157,
       -1, 158,  -1, 159, 160,  -1, 161, 162,  -1, 163,
      164, 165,  -1,  -1, 166,  -1, 167,  -1,  -1, 168,
      169, 170,  -1,  -1, 171,  -1, 172,  -1, 173, 174,
      175, 176,  -1,  -1, 177, 178, 179,  -1,  -1, 180,
      181,  -1,  -1, 182, 183, 184, 185,  -1, 186,  -1,
       -1,  -1,  -1,  -1, 187, 188,  -1,  -1,  -1, 189,
      190, 191,  -1,  -1, 192, 193, 194, 195,  -1,  -1,
      196, 197,  -1,  -1, 198, 199, 200,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 201, 202,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 203,  -1,  -1,  -1,  -1, 204,
      205, 206,  -1,  -1, 207, 208, 209,  -1,  -1, 210,
       -1,  -1,  -1, 211,  -1,  -1, 212,  -1, 213, 214,
       -1,  -1,  -1,  -1,  -1, 215,  -1,  -1,  -1, 216,
      217,  -1,  -1, 218,  -1,  -1, 219,  -1,  -1, 220,
       -1,  -1,  -1,  -1,  -1, 221,  -1,  -1, 222, 223,
      224,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1, 225,  -1,  -1, 226, 227,  -1,  -1,  -1, 228,
      229,  -1,  -1, 230, 231, 232,  -1,  -1,  -1,  -1,
      233,  -1,  -1,  -1,  -1, 234,  -1,  -1,  -1, 235,
      236, 237,  -1,  -1, 238,  -1, 239,  -1,  -1,  -1,
      240, 241,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      242,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 243,  -1,  -1,  -1,  -1,
      244,  -1, 245,  -1, 246, 247,  -1,  -1, 248,  -1,
       -1,  -1,  -1,  -1, 249,  -1, 250,  -1,  -1,  -1,
       -1, 251,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 252,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1, 253,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 254,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      255
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = htmlentities (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist[index];
            }
        }
    }
  return 0;
}
