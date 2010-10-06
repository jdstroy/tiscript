/* C++ code produced by gperf version 3.0.1 */
/* Command-line: c:/utils/gperf.exe -t -L C++ -H encodings -N find_def -Z html_encodings -D html_encodings.txt  */
/* Computed positions: -k'4,9-10,$' */

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

#line 1 "html_encodings.txt"
struct html_encoding_def { char *name; uint value; };

#define TOTAL_KEYWORDS 116
#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 33
#define MIN_HASH_VALUE 6
#define MAX_HASH_VALUE 285
/* maximum key range = 280, duplicates = 0 */

class html_encodings
{
private:
  static inline unsigned int encodings (const char *str, unsigned int len);
public:
  static struct html_encoding_def *find_def (const char *str, unsigned int len);
};

inline unsigned int
html_encodings::encodings (register const char *str, register unsigned int len)
{
  static unsigned short asso_values[] =
    {
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286,   0, 286, 286, 125,  25,
       20, 100,   2,  30,  85,  10,   5,  90, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286,  30,   5,  25,
        0,  35, 120,  90,  40,   5,  20,  70,   0, 110,
        0,   0,  45, 286,  50,  15,  25,  85, 286,  75,
      286,  85, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286, 286, 286, 286, 286,
      286, 286, 286, 286, 286, 286
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
      case 7:
      case 6:
      case 5:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct html_encoding_def *
html_encodings::find_def (register const char *str, register unsigned int len)
{
  static struct html_encoding_def wordlist[] =
    {
#line 15 "html_encodings.txt"
      {"euc-cn",51936},
#line 110 "html_encodings.txt"
      {"utf-8",65001},
#line 3 "html_encodings.txt"
      {"asmo-708",708},
#line 9 "html_encodings.txt"
      {"iso-8859-4",28594},
#line 109 "html_encodings.txt"
      {"utf-7",65000},
#line 102 "html_encodings.txt"
      {"ibm437",437},
#line 38 "html_encodings.txt"
      {"iso-8859-8",28598},
#line 105 "html_encodings.txt"
      {"ibm857",857},
#line 37 "html_encodings.txt"
      {"iso-8859-8-i",38598},
#line 31 "html_encodings.txt"
      {"ibm737",737},
#line 36 "html_encodings.txt"
      {"dos-862",862},
#line 104 "html_encodings.txt"
      {"windows-874",874},
#line 58 "html_encodings.txt"
      {"x-ebcdic-italy-euro",1144},
#line 32 "html_encodings.txt"
      {"iso-8859-7",28597},
#line 11 "html_encodings.txt"
      {"ibm852",852},
#line 45 "html_encodings.txt"
      {"x-ebcdic-denmarknorway-euro",1142},
#line 55 "html_encodings.txt"
      {"x-ebcdic-icelandic-euro",1149},
#line 67 "html_encodings.txt"
      {"x-ebcdic-spain",20284},
#line 77 "html_encodings.txt"
      {"ibm861",861},
#line 56 "html_encodings.txt"
      {"x-ebcdic-international-euro",1148},
#line 68 "html_encodings.txt"
      {"x-ebcdic-spain-euro",1145},
#line 97 "html_encodings.txt"
      {"johab",1361},
#line 111 "html_encodings.txt"
      {"us-ascii",1252},
#line 8 "html_encodings.txt"
      {"ibm775",775},
#line 69 "html_encodings.txt"
      {"x-ebcdic-thai",20838},
#line 76 "html_encodings.txt"
      {"x-ebcdic-cp-us-euro",1140},
#line 12 "html_encodings.txt"
      {"iso-8859-2",28592},
#line 89 "html_encodings.txt"
      {"euc-jp",51932},
#line 22 "html_encodings.txt"
      {"x-mac-chinesetrad",10002},
#line 54 "html_encodings.txt"
      {"x-ebcdic-icelandic",20871},
#line 42 "html_encodings.txt"
      {"x-ebcdic-cyrillicrussian",20880},
#line 95 "html_encodings.txt"
      {"euc-kr",51949},
#line 60 "html_encodings.txt"
      {"x-ebcdic-japaneseandjapaneselatin",50939},
#line 108 "html_encodings.txt"
      {"windows-1254",1254},
#line 115 "html_encodings.txt"
      {"iso-8859-1",1252},
#line 25 "html_encodings.txt"
      {"koi8-r",20866},
#line 112 "html_encodings.txt"
      {"windows-1258",1258},
#line 43 "html_encodings.txt"
      {"x-ebcdic-cyrillicserbianbulgarian",21025},
#line 19 "html_encodings.txt"
      {"big5",950},
#line 114 "html_encodings.txt"
      {"x-ia5",20105},
#line 100 "html_encodings.txt"
      {"iso-8859-15",28605},
#line 10 "html_encodings.txt"
      {"windows-1257",1257},
#line 24 "html_encodings.txt"
      {"iso-8859-5",28595},
#line 75 "html_encodings.txt"
      {"ebcdic-cp-us",37},
#line 13 "html_encodings.txt"
      {"x-mac-ce",10029},
#line 94 "html_encodings.txt"
      {"ks_c_5601-1987",949},
#line 41 "html_encodings.txt"
      {"x-ebcdic-arabic",20420},
#line 90 "html_encodings.txt"
      {"iso-2022-jp",50222},
#line 117 "html_encodings.txt"
      {"windows-1252",1252},
#line 59 "html_encodings.txt"
      {"x-ebcdic-japaneseandkana",50930},
#line 62 "html_encodings.txt"
      {"x-ebcdic-japanesekatakana",20290},
#line 66 "html_encodings.txt"
      {"x-ebcdic-simplifiedchinese",50935},
#line 28 "html_encodings.txt"
      {"windows-1251",1251},
#line 61 "html_encodings.txt"
      {"x-ebcdic-japaneseanduscanada",50931},
#line 79 "html_encodings.txt"
      {"x-iscii-as",57006},
#line 72 "html_encodings.txt"
      {"x-ebcdic-turkish",20905},
#line 40 "html_encodings.txt"
      {"windows-1255",1255},
#line 21 "html_encodings.txt"
      {"x-chinese-eten",20002},
#line 70 "html_encodings.txt"
      {"x-ebcdic-traditionalchinese",50937},
#line 116 "html_encodings.txt"
      {"macintosh",10000},
#line 81 "html_encodings.txt"
      {"x-iscii-de",57002},
#line 26 "html_encodings.txt"
      {"koi8-u",21866},
#line 18 "html_encodings.txt"
      {"x-mac-chinesesimp",10008},
#line 64 "html_encodings.txt"
      {"x-ebcdic-koreanextended",20833},
#line 80 "html_encodings.txt"
      {"x-iscii-be",57003},
#line 35 "html_encodings.txt"
      {"ibm869",869},
#line 6 "html_encodings.txt"
      {"x-mac-arabic",10004},
#line 20 "html_encodings.txt"
      {"x-chinese-cns",20000},
#line 78 "html_encodings.txt"
      {"x-mac-icelandic",10079},
#line 74 "html_encodings.txt"
      {"x-ebcdic-uk-euro",1146},
#line 63 "html_encodings.txt"
      {"x-ebcdic-koreanandkoreanextended",50933},
#line 57 "html_encodings.txt"
      {"x-ebcdic-italy",20280},
#line 87 "html_encodings.txt"
      {"x-iscii-ta",57004},
#line 91 "html_encodings.txt"
      {"csiso2022jp",50221},
#line 44 "html_encodings.txt"
      {"x-ebcdic-denmarknorway",20277},
#line 51 "html_encodings.txt"
      {"x-ebcdic-greekmodern",875},
#line 50 "html_encodings.txt"
      {"x-ebcdic-germany-euro",1141},
#line 103 "html_encodings.txt"
      {"x-ia5-swedish",20107},
#line 88 "html_encodings.txt"
      {"x-iscii-te",57005},
#line 29 "html_encodings.txt"
      {"x-europa",29001},
#line 27 "html_encodings.txt"
      {"x-mac-cyrillic",10007},
#line 85 "html_encodings.txt"
      {"x-iscii-or",57007},
#line 16 "html_encodings.txt"
      {"gb2312",936},
#line 98 "html_encodings.txt"
      {"x-mac-korean",10003},
#line 86 "html_encodings.txt"
      {"x-iscii-pa",57011},
#line 96 "html_encodings.txt"
      {"iso-2022-kr",50225},
#line 4 "html_encodings.txt"
      {"dos-720",720},
#line 53 "html_encodings.txt"
      {"x-ebcdic-hebrew",20424},
#line 113 "html_encodings.txt"
      {"ibm850",850},
#line 65 "html_encodings.txt"
      {"cp870",870},
#line 118 "html_encodings.txt"
      {"system",0},
#line 7 "html_encodings.txt"
      {"windows-1256",1256},
#line 48 "html_encodings.txt"
      {"x-ebcdic-france-euro",1147},
#line 46 "html_encodings.txt"
      {"x-ebcdic-finlandsweden",20278},
#line 47 "html_encodings.txt"
      {"x-ebcdic-finlandsweden-euro",1143},
#line 92 "html_encodings.txt"
      {"x-mac-japanese",10001},
#line 83 "html_encodings.txt"
      {"x-iscii-ka",57008},
#line 34 "html_encodings.txt"
      {"windows-1253",1253},
#line 93 "html_encodings.txt"
      {"shift_jis",932},
#line 17 "html_encodings.txt"
      {"hz-gb-2312",52936},
#line 101 "html_encodings.txt"
      {"x-ia5-norwegian",20108},
#line 73 "html_encodings.txt"
      {"x-ebcdic-uk",20285},
#line 39 "html_encodings.txt"
      {"x-mac-hebrew",10005},
#line 23 "html_encodings.txt"
      {"cp866",866},
#line 52 "html_encodings.txt"
      {"x-ebcdic-greek",20423},
#line 5 "html_encodings.txt"
      {"iso-8859-6",28596},
#line 33 "html_encodings.txt"
      {"x-mac-greek",10006},
#line 14 "html_encodings.txt"
      {"windows-1250",1250},
#line 106 "html_encodings.txt"
      {"iso-8859-9",28599},
#line 84 "html_encodings.txt"
      {"x-iscii-ma",57009},
#line 49 "html_encodings.txt"
      {"x-ebcdic-germany",20273},
#line 30 "html_encodings.txt"
      {"x-ia5-german",20106},
#line 107 "html_encodings.txt"
      {"x-mac-turkish",10081},
#line 99 "html_encodings.txt"
      {"iso-8859-3",28593},
#line 71 "html_encodings.txt"
      {"cp1026",1026},
#line 82 "html_encodings.txt"
      {"x-iscii-gu",57010}
    };

  static signed char lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,   0,  -1,  -1,  -1,
        1,  -1,  -1,   2,   3,   4,  -1,  -1,   5,  -1,
        6,   7,   8,  -1,  -1,  -1,   9,  10,  11,  12,
       13,  14,  15,  16,  17,  -1,  18,  19,  -1,  20,
       21,  -1,  -1,  22,  -1,  -1,  23,  -1,  24,  25,
       26,  27,  28,  29,  30,  -1,  31,  -1,  32,  33,
       34,  35,  36,  37,  38,  39,  40,  41,  -1,  -1,
       42,  -1,  43,  44,  45,  46,  47,  48,  -1,  49,
       50,  51,  52,  53,  -1,  54,  55,  56,  -1,  57,
       -1,  -1,  58,  -1,  59,  60,  61,  62,  63,  -1,
       64,  65,  66,  67,  -1,  68,  69,  70,  -1,  71,
       72,  73,  74,  -1,  -1,  75,  76,  -1,  77,  -1,
       78,  -1,  -1,  79,  80,  81,  82,  83,  -1,  -1,
       84,  85,  86,  -1,  -1,  87,  88,  -1,  -1,  -1,
       89,  90,  91,  -1,  -1,  92,  -1,  93,  -1,  -1,
       -1,  -1,  94,  -1,  95,  96,  -1,  97,  -1,  98,
       -1,  -1,  -1,  -1,  -1,  99,  -1,  -1,  -1,  -1,
      100, 101, 102,  -1,  -1, 103,  -1,  -1,  -1, 104,
      105, 106, 107,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      108,  -1,  -1,  -1,  -1, 109, 110,  -1,  -1,  -1,
       -1,  -1, 111, 112,  -1,  -1,  -1,  -1,  -1,  -1,
      113,  -1,  -1,  -1,  -1,  -1, 114,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 115
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = encodings (str, len);

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
