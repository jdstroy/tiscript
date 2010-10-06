#ifndef TL_WREGEXP_H_
#define TL_WREGEXP_H_

#include "tl_config.h"
#include "tl_slice.h"
#include "tl_array.h"
#include "tl_ustring.h"

#include <stddef.h>
#include <wchar.h>
#include "rp_regexp.h"

namespace tool
{
  struct regmatch_r
  {
    int begin;
    int end;
    regmatch_r():begin(-1),end(-1) {}
  };

  class wregexp
  {
  public:
    enum _ 
    {
      NSUBEXP = 64 // number of subexpressions
    };
  protected:
      void reset_matches();

      reimpl::Reprog* m_compiled;
      array<regmatch_r> m_matches;
      reimpl::Resub   m_rsubs[NSUBEXP];

  public:
      ustring       m_pattern;
      ustring       m_test;
      ustring       m_test_input; // if 'i' (m_ignorecase) then this is lowercase version of the m_test 
                                  // otherwise m_test itself
      int           m_next_index;
      int           m_index;
      ustring       m_error;
      bool          m_ignorecase;
      bool          m_global;

      wregexp():m_compiled(0),m_ignorecase(false),m_global(false) {}
      wregexp(const wchar* pattern, bool ignorecase = true, bool global = true): m_ignorecase(ignorecase),m_global(global) 
      { compile(pattern,ignorecase,global); }
      ~wregexp();


      bool compile(const wchar* pattern, bool ignorecase, bool global);
      bool exec(const wchar* text);
      bool exec_all(const wchar* text);
     
      bool is_matched(int nSubExp = 0) const;
      int  get_match_start(int nSubExp = 0) const;
      int  get_match_end(int nSubExp = 0) const;
      wchars get_match(int nSubExp = 0) const;
      regmatch_r  get_n_match(int n = 0) const;

      wchars  text() { return m_test; }

      // substitute string using the matches from the last exec()
      ustring subs(wchars text);

      int  get_number_of_matches() const;

      ustring get_error_string() const;


  private:
      // disable copying
      wregexp(const wregexp&);
      wregexp& operator=(const wregexp&);
  };

}

#endif /* TL_WREGEXP_H_ */
