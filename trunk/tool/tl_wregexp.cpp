#include "tl_wregexp.h"

namespace tool
{
  bool wregexp::compile(const wchar* psz_pattern, bool ignorecase, bool global)
  {
      m_pattern = psz_pattern;
      tool::ustring pat = m_pattern;
      m_ignorecase = ignorecase;
      m_global = global;
      m_next_index = 0;
      if(m_ignorecase)
        pat.to_lower();
      m_test.clear();
      m_matches.clear();
      memset(m_rsubs,0,sizeof(m_rsubs));

      try 
      {
        m_compiled = reimpl::regcomp(pat);
      }
      catch(reimpl::ReError& er)
      {
        m_error = er.msg;
        reimpl::regfree(m_compiled);
        m_compiled = 0;
        return false;
      }
      return true;
  }

  tool::ustring wregexp::get_error_string() const
  {
      return m_error;
  }

  wregexp::~wregexp()
  {
      reimpl::regfree(m_compiled);
  }

  bool wregexp::exec(const wchar* sMatch)
  {
      tool::ustring tst = m_test;   
      
      if(sMatch)
      {
        m_test = sMatch;
        tst = m_test;
        tool::ustring cvt;   
        if(m_ignorecase)
        {
          cvt = m_test;
          cvt.to_lower();
          tst = cvt;
        }
      }
      
      m_index = m_global? m_next_index: 0;

      m_matches.clear();
      memset(m_rsubs,0,sizeof(m_rsubs));
      
      if(m_index < 0 || m_index >= tst.length())
      {
        m_next_index = 0;
        return false;
      }
      
      int res = tool::reimpl::regexec(m_compiled, tst.c_str() + m_index, m_rsubs, NSUBEXP);
      if( res <= 0) 
      { 
        m_next_index = 0; 
        return false; 
      }

      m_next_index = m_rsubs[0].ep - tst.c_str();
        
      for(int n = 0; n < NSUBEXP; ++n)
      {
        if( m_rsubs[n].sp && m_rsubs[n].ep )
        {
          m_matches.size( n + 1 );
          regmatch_r rm;
          rm.begin = m_rsubs[n].sp - tst.c_str();
          rm.end = m_rsubs[n].ep - tst.c_str();
          m_matches[n] = rm;
        }
      }
      return m_matches.size() > 0;
  }

  bool wregexp::exec_all(const wchar* sMatch)
  {
      m_test = sMatch;
      tool::ustring tst = m_test; 
      tool::ustring cvt;   
      if(m_ignorecase)
      {
        cvt = m_test;
        cvt.to_lower();
        tst = cvt;
      }
      
      m_next_index = 0;

      m_matches.clear();
      
      while(true)
      {
        memset(m_rsubs,0,sizeof(m_rsubs));
        m_index = m_next_index;
        if(m_index < 0 || m_index >= tst.length())
          break;
        int res = tool::reimpl::regexec(m_compiled, tst.c_str() + m_index, m_rsubs, NSUBEXP);
        if( res <= 0) 
        { 
          m_next_index = 0; 
          break; 
        }
        m_next_index = m_rsubs[0].ep - tst.c_str();
        if( m_rsubs[0].sp && m_rsubs[0].ep )
        {
          regmatch_r rm;
          rm.begin = m_rsubs[0].sp - tst.c_str();
          rm.end = m_rsubs[0].ep - tst.c_str();
          m_matches.push(rm);
        }
      }
      m_next_index = 0;
      return m_matches.size() > 0;
  }


  bool wregexp::is_matched(int nSubExp) const
  {
      return (nSubExp < m_matches.size() && m_matches[nSubExp].begin != -1);
  }

  int wregexp::get_match_start(int matchNo) const
  {
     return m_matches[matchNo].begin;
  }

  int wregexp::get_match_end(int matchNo) const
  {
    return m_matches[matchNo].end;
  }

  tool::wchars wregexp::get_match(int matchNo) const
  {
      if(matchNo >= m_matches.size())
          return tool::wchars();

      regmatch_r rmMatch = m_matches[matchNo];
      
      if(rmMatch.begin == -1 || rmMatch.end == -1)
          return tool::wchars();

      return tool::wchars( m_test.c_str() + rmMatch.begin, rmMatch.end - rmMatch.begin );
  }

  regmatch_r wregexp::get_n_match(int matchNo) const
  {
      if(matchNo >= m_matches.size())
      {
          regmatch_r rm; rm.begin = rm.end = -1;
          return rm;
      }
      return m_matches[matchNo];
  }

  int wregexp::get_number_of_matches() const
  {
    return m_matches.size();
  }

  /* substitute string using the matches from the last regexec() */
  ustring wregexp::subs(wchars text)
  {
	  const wchar *ssp;
	  int i;
    array<wchar> out;
    const wchar* sp = text.start;
    const wchar* spend = text.end();
  	
  	while(sp != spend)
    {
      if( *sp != '$' )
      {
        out.push(*sp);
        ++sp;
        continue;
      }
      if( ++sp == spend )
			  break;
      switch(*sp)
      {
			  case '0': case '1':	case '2':	case '3':	case '4':
			  case '5':	case '6':	case '7':	case '8':	case '9':
				  i = *sp-'0';
				  if(m_rsubs[i].sp != 0 && i < NSUBEXP)
					  for(ssp = m_rsubs[i].sp; ssp < m_rsubs[i].ep; ssp++)
						  out.push(*ssp);
				  break;
			  case '$':
          out.push('$');
  				break;
        case '&':
			    if(m_rsubs[0].sp != 0)
				    for(ssp = m_rsubs[0].sp; ssp < m_rsubs[0].ep; ssp++)
					    out.push(*ssp);
          break;
			  default:
          out.push(*sp);
 				  break;
			}
  		++sp;
	  }
    return out();
  }

}


