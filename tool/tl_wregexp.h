#ifndef TL_WREGEXP_H_
#define TL_WREGEXP_H_

#include "tl_config.h"
#include "tl_slice.h"
#include "tl_array.h"
#include "tl_ustring.h"

#include <stddef.h>

/*
 * Definitions etc. for regexp(3) routines.
 *
 * NOTE: this was completely redone from the old code to have a
 * saner interface (with pretty much the same functionality).
 */

#include <wchar.h>

typedef int regoffset;
typedef struct regmatch_w
{
    regoffset begin;
    regoffset end;
} regmatch_w;

struct tag_regexp_w;
typedef struct tag_regexp_w regexp_w;

/* error codes -- similar to POSIX codes */
#define REGEXP_BADARG  -1         /* bad argument--NULL pointer or such */
#define REGEXP_ESIZE   -2         /* regexp too big */
#define REGEXP_ESPACE  -3         /* out of memory */
#define REGEXP_EPAREN  -4         /* unmatched ) */
#define REGEXP_ERANGE  -5         /* invalid [] range */
#define REGEXP_EBRACK  -6         /* unclosed ] */
#define REGEXP_BADRPT  -7         /* *+? follows nothing */
#define REGEXP_EESCAPE -8         /* trailing backslash */
#define REGEXP_EEND   -99         /* unspecified internal error */
  
extern void re_error(int errcode, const regexp_w* re, char *buffer, size_t bufsize);
extern int re_nsubexp(const regexp_w *rp);
extern void re_free(void* object);

/* Unicode-dependent */
extern int re_comp_w(regexp_w **rpp, const wchar* regex);
extern int re_exec_w(const regexp_w *rp, const wchar *s, size_t nmatch, regmatch_w pmatch[]);
extern int re_sub_w(const regexp_w *rp, const wchar* s, const wchar *src, wchar** sub);
extern int re_subcount_w(const regexp_w* rp, const wchar* s, const wchar* src, regmatch_w matches[10]);
extern int re_dosub_w(const wchar* s, const wchar* src, regmatch_w matches[10], wchar* dest);

namespace tool
{
   
  class wregexp
  {
  public:
      wregexp():m_preCompiled(0),m_nError(0),m_ignorecase(false),m_global(false) {}
      ~wregexp();

      bool compile(const wchar* pattern, bool ignorecase, bool global);
      bool exec(const wchar* text);
      bool is_matched(int nSubExp = 0) const;
      int  get_match_start(int nSubExp = 0) const;
      int  get_match_end(int nSubExp = 0) const;
      ustring get_match(int nSubExp = 0) const;
      wchars  get_n_match(int n = 0) const;

      wchars  text() { return tool::wchars(m_test,m_test.length()); }  

      int  get_number_of_matches() const;

      string get_error_string() const;

      ustring       m_pattern;
      ustring       m_test;

      int           m_nextIndex;
      int           m_index;
      int           m_nError;
      bool          m_ignorecase;
      bool          m_global;

  protected:
      void reset_matches();
      bool is_error(int nErrorCode);
      
      regexp_w*     m_preCompiled;
      tool::array<regmatch_w> m_arMatches;
      tool::array<regmatch_w> m_result;

  private:
      // disable copying
      wregexp(const wregexp&);
      wregexp& operator=(const wregexp&);
  };

}

#endif /* TL_WREGEXP_H_ */
