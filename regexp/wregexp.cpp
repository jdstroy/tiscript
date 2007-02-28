#include "tool.h"
// include the C regexp code
//#ifdef _UNICODE

#include "wregexp.h"

bool wregexp::compile(const wchar* pszPattern, bool ignorecase, bool global)
{
    m_pattern = pszPattern;
    tool::ustring pat = m_pattern;
    m_ignorecase = ignorecase;
    m_global = global;
    m_nextIndex = 0;
    if(m_ignorecase)
      pat.to_lower();
    m_test.clear();
    int nErrCode = re_comp_w(&m_preCompiled, pat);
    if(!is_error(nErrCode))
    {
        nErrCode = re_nsubexp(m_preCompiled);
        if(!is_error(nErrCode))
        {
          m_arMatches.size(nErrCode);
          reset_matches();
          return true;
        }
    }
    re_free(m_preCompiled);
    m_preCompiled = 0;
    return false;
}

tool::string wregexp::get_error_string() const
{
    if(m_nError >= 0)
      return tool::string(); 
    char arTemp[128];
    re_error(m_nError, NULL, arTemp, sizeof(arTemp));
    tool::string retval(arTemp);
    return retval;
}

wregexp::~wregexp()
{
    re_free(m_preCompiled);
}

bool wregexp::exec(const wchar* sMatch)
{
    m_test = sMatch;
    tool::ustring tst = m_test;   
    if(m_ignorecase)
      tst.to_lower();

    m_result.size(0);

    while( true )
    {
      reset_matches();

      m_index = m_global? m_nextIndex: 0;

      if(m_index < 0 || m_index >= tst.length())
      {
        m_nextIndex = 0;
        break;
      }

      int nErrCode = re_exec_w(m_preCompiled,
                               (const wchar*)tst + m_index,
                               m_arMatches.size(),
                               &m_arMatches[0]);

      is_error(nErrCode);
      if( nErrCode <= 0) { m_nextIndex = 0; break; }
            
      regmatch rm;
      rm.begin = m_index + m_arMatches[0].begin;
      m_nextIndex = rm.end = m_index + m_arMatches[0].end;
      m_result.push(rm);

      if(!m_global)
        break;

    }

    return m_result.size() > 0;

}


bool wregexp::is_matched(int nSubExp) const
{
    return (nSubExp < m_arMatches.size() && m_arMatches[nSubExp].begin != -1);
}

int wregexp::get_match_start(int matchNo) const
{
   return m_result[matchNo].begin;
    //return nSubExp >= m_arMatches.size() ? -1 : m_index + m_arMatches[nSubExp].begin;
}

int wregexp::get_match_end(int matchNo) const
{
  return m_result[matchNo].end;
    //return nSubExp >= m_arMatches.size() ?  -1 : m_index + m_arMatches[nSubExp].end;
}

tool::ustring wregexp::get_match(int matchNo) const
{
    if(matchNo >= m_result.size())
        return tool::ustring();

    regmatch rmMatch = m_result[matchNo];
    
    if(rmMatch.begin == -1 || rmMatch.end == -1)
        return tool::ustring();

    return m_test.substr(rmMatch.begin, rmMatch.end - rmMatch.begin);
}

tool::wchars wregexp::get_n_match(int matchNo) const
{
    if(matchNo >= m_result.size())
        return tool::wchars();

    regmatch rmMatch = m_result[matchNo];
    
    if(rmMatch.begin == -1 || rmMatch.end == -1)
        return tool::wchars();

    return tool::wchars( ((const wchar*)m_test) + rmMatch.begin, ((const wchar*)m_test) + rmMatch.end);
}



int wregexp::get_number_of_matches() const
{
	return m_result.size();
}

void wregexp::reset_matches()
{
    regmatch rmDummy;
    rmDummy.begin = rmDummy.end = -1;
    int nSize = m_arMatches.size();

    for(int nIndex = 0; nIndex < nSize; ++nIndex)
        m_arMatches[nIndex] = rmDummy;
}

bool wregexp::is_error(int nErrorCode)
{
    if(nErrorCode < 0)
    {
      m_nError = nErrorCode;
      return true;
    }
    m_nError = 0;
    return false;
        
}

extern "C" void re_report(const char* error) {}
