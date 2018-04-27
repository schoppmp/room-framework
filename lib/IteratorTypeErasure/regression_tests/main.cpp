//  (C) Copyright Thomas Becker 2005. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Revision History
// ================
//
// 27 Dec 2006 (Thomas Becker) Created

// Includes
// ========

#include <stdlib.h>

#ifdef _MSC_VER

  #include <windows.h>
  #include <tchar.h>

  // Enable rudimentary memory tracking under Win32.
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>

  // Disable stupid warning about what Microsoft thinks is deprecated
  #pragma warning( disable : 4996 )

#else

  #include<iostream>

#endif

void any_iterator_demo();
bool test_any_iterator_wrapper();
bool test_any_iterator();
void boost_iterator_library_example_test();

#ifdef _MSC_VER

int _tmain(int argc, _TCHAR* argv[])

#else

int main(int argc, char* argv[])

#endif
{

// Enable rudimentary memory tracking under Win32.
#ifdef _MSC_VER
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
  tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag( tmpFlag );
#endif

  any_iterator_demo();

  if( ! test_any_iterator_wrapper() )
  {
#ifdef _MSC_VER
    ::MessageBox(NULL, _T("Problem in test_any_iterator_wrapper."), _T("Error"), 0);
#else
    std::cout << "Problem in test_any_iterator_wrapper." << std::endl;
#endif
  }

  if( ! test_any_iterator() )
  {
#ifdef _MSC_VER
    ::MessageBox(NULL, _T("Problem in test_any_iterator."), _T("Error"), 0);
#else
    std::cout << "Problem in test_any_iterator." << std::endl;
#endif
  }

  boost_iterator_library_example_test();
  
  return 0;

}
