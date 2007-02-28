// -*- C++ -*-

#include "tool.h"
#include "regexp.h"

class wregexp
{
public:
    wregexp():m_preCompiled(0),m_nError(0),m_ignorecase(false),m_global(false) {}
    ~wregexp();

    bool compile(const wchar* pszPattern, bool ignorecase, bool global);
    bool exec(const wchar* sMatch);
    bool is_matched(int nSubExp = 0) const;
    int  get_match_start(int nSubExp = 0) const;
    int  get_match_end(int nSubExp = 0) const;
    tool::ustring get_match(int nSubExp = 0) const;
    tool::wchars  get_n_match(int n = 0) const;

    tool::wchars  text() { return tool::wchars(m_test,m_test.length()); }  

	  int  get_number_of_matches() const;

    tool::string get_error_string() const;

    tool::ustring m_pattern;
    tool::ustring m_test;

    int           m_nextIndex;
    int           m_index;
    int           m_nError;
    bool          m_ignorecase;
    bool          m_global;

protected:
    void reset_matches();
    bool is_error(int nErrorCode);
    
    typedef regexp *PREGEXP;
    PREGEXP       m_preCompiled;
    tool::array<regmatch> m_arMatches;
    
    tool::array<regmatch> m_result;


private:
    // disable copying
    wregexp(const wregexp&);
    wregexp& operator=(const wregexp&);
};

