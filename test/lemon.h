///////////////////////////////////////////////////////////////////////////////
// lemon.h - A minimal unit-testing framework based on Test::More
//
// Description:
//   lemon is a minimal unit-testing framework designed to be simple to drop
// in and use without require external dependencies or linking. In this way
// lemon hopes to promote testing on projects of all sizes by reducing the
// barrier of setup that comes with most unit-testing frameworks. To find
// the latest version visit http://github.com/etscrivner/lemon.
//
// Copyright (c) 2010 lemon team
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
// distribution.
///////////////////////////////////////////////////////////////////////////////
#ifndef LEMON_H_
#define LEMON_H_

// C++ includes
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Macro: LEMON_SKIP
//
// Begins a block of tests that will be skipped, creating a more convenient
// syntax for test skipping. For example:
//
// <code>
// lemon::test<> lemon(..);
// // ...
// LEMON_SKIP(lemon, "These tests are currently broken") {
//   lemon.is(foo(), true, "foo true");
//   lemon.is(bar(), false, "bar false");
// }
// </code>
#define LEMON_SKIP(lemon_inst, reason) \
  for (bool __skip_enabled__ = (lemon_inst).enable_skip(); \
       __skip_enabled__; __skip_enabled__ = (lemon_inst).disable_skip())

///////////////////////////////////////////////////////////////////////////////
// Macro: LEMON_TODO
//
// Begins a block of tests for features which have yet to be completed. For
// example:
//
// <code>
// lemon::test<> lemon(..);
// // ...
// LEMON_TODO(lemon) {
//   lemon.is(tumtum(), fizbaz(), "tumtum is fizbaz");
//   lemon.not_ok(cannibalism(), "cannibalism is not ok");
// }
// </code>
#define LEMON_TODO(lemon_inst) \
  for (bool __todo_enabled__ = (lemon_inst).enable_todo(); \
       __todo_enabled__; __todo_enabled__ = (lemon_inst).disable_todo())

namespace lemon {
  namespace output {
    ///////////////////////////////////////////////////////////////////////////
    // Class: cout
    //
    // Implements output to standard out
    struct cout {
      template<typename T>
      lemon::output::cout& operator << (const T& val) {
        std::cout << val;
        return *this;
      }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Class: cerr
    //
    // Implements output to standard error
    struct cerr {
      template<typename T>
      lemon::output::cerr& operator << (const T& val) {
        std::cerr << val;
        return *this;
      }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Class: clog
    //
    // Implements output to standard logging
    struct clog {
      template<typename T>
      lemon::output::clog& operator << (const T& val) {
        std::clog << val;
        return *this;
      }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    // Class: nothing
    //
    // Implements a null output policy
    struct nothing {
      template<typename T>
      lemon::output::nothing& operator << (const T& val) {
        return *this;
      }
    };
  }

  /////////////////////////////////////////////////////////////////////////////
  // Class: test
  //
  // Policy-based class for doing testing. For example a simple test might be:
  //
  // <code>
  // bool always_true() { return true; }
  // //...
  // lemon::test<> lemon(2);
  // lemon.is(always_true(), true, "always_true is true");
  // lemon.isnt(always_true(), false, "always_true isn't false");
  // lemon.done();
  // </code>
  template <class output_policy_t = lemon::output::cout>
  class test {
  public:
  /////////////////////////////////////////////////////////////////////////////
  // Function: test
  //
  // Parameters:
  //    num_planned_tests - The total number of tests you plan to execute
  //
  // This simply lets lemon know how many tests you're planning to run so that
  // it can properly output the diagnostic information and doesn't have to
  // count by hand (which can be tricky as one test can have many assertions).
  test (unsigned int num_planned_tests = 0)
  : num_tests_(0),
    test_number_(0),
    num_skipped_(0),
    num_failed_(0),
    num_planned_(num_planned_tests),
    skip_enabled_(false),
    todo_enabled_(false)
  {
    if (num_planned_tests > 0) {
      output_ << "1.." << num_planned_tests << "\n";
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Function: done
  //
  // Returns true if all unskipped tests passed, false if there were failures.
  bool done () {
    // If there was no test plan specified
    if (num_planned_ == 0) {
      output_ << "1.." << num_tests_ << "\n";
    }

    // Compute the total number of tests without skipped tests counted
    unsigned int total_tests = num_tests_ - num_skipped_;

    // If any tests were skipped
    if (num_planned_ > 0 && num_skipped_ > 0) {
      // Display information about the skipped tests
      output_ << "# Looks like you planned " << num_planned_;
      output_ << " but only ran " << total_tests << "\n";
    }
    
    // If any tests were failed
    if (num_failed_ > 0) {
      // Display test failure statistics
      output_ << "# Looks like you failed " << num_failed_;
      output_ << " of " << total_tests << "\n";
      return false;
    } else if(num_planned_ > 0 && total_tests > num_planned_) {
      output_ << "# Looks like you ran " << total_tests << " tests, ";
      output_ << "but only planned " <<  num_planned_ << "\n";
      return false;
    } else {
      // Otherwise display success message
      output_ << "# Looks like you passed all " << total_tests << " tests.\n";
      return true;
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Function: diag
  //
  // Parameters:
  //    message - A string to be written out to the display
  //
  // Used to display diagnostic information which is not a unit test.
  void diag (const std::string& message) {
    output_ << "# " << message << "\n";
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Function: ok
  //
  // Parameters:
  //    passed - True indicates a passing condition, false indicates failure
  //    test_name - A short, descriptive name for this test
  //
  // Marks this test as passed if pass is true.  The test is marked as
  // failing otherwise.
  bool ok (bool passed, const std::string& test_name) {
    // Increment the number of tests run
    num_tests_++;
    
    // If this is a skip or todo message
    std::string test_name_out = test_name;
    if (test_name[0] != '#') {
      // Not the safest thing, but append a dash to the front
      test_name_out = "- " + test_name_out;
    }

    // If we're currently skipping tests
    if (skip_enabled_) {
      num_skipped_++;
      output_ << "skipping " << num_tests_ << " " << test_name_out << "\n";
      return false;
    } else if (todo_enabled_) {
      num_skipped_++;
      output_ << "todo " << num_tests_ << " " << test_name_out << "\n";
      return false;
    } else if (passed) {
      // Inform you that the test passed
      output_ << "ok " << num_tests_ << " " << test_name_out << "\n";
    } else {
      // Otherwise increment the number of failed tests.
      num_failed_++;
      
      // Inform you that the test failed
      output_ << "not ok " << num_tests_ << " " << test_name_out << "\n";
      diag("  Failed test '" + test_name + "'");
    }
  
    return passed;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Function: not_ok
  //
  // Parameters:
  //    failed - False indicates a passing condition, true indicates failure
  //    test_name - A short, descriptive name for this test
  //
  // Marks this test as passed if the boolean parameter is false. The test is 
  // marked as failing otherwise.
  bool not_ok (bool failed, const std::string& test_name) {
    return ok(!failed, test_name);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: is
  //
  // Parameters:
  //    this_one - The left hand of the equality operator
  //    that_one - The right hand of the equality operator
  //    test_name - A short, descriptive name for this test
  //
  // Checks whether the two values are equal using the == operator. If
  // they are equal the test passes, otherwise it fails.
  template<typename T1, typename T2>
  bool is (const T1& this_one,
           const T2& that_one,
           const std::string& test_name)
  {
    bool passed = (this_one == that_one);
      
    ok(passed, test_name);
      
    if (!passed && !skip_enabled_ && !todo_enabled_) {
      output_ << "#         got: '" << this_one << "'\n";
      output_ << "#    expected: '" << that_one << "'\n";
    }
      
    return passed;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: isnt
  //
  // Parameters:
  //    this_one - The left hand of the inequality operator
  //    that_one - The right hand of the inequality operator
  //    test_name - A short, descriptive name for this test
  //
  // Checks whether the two values are equal using the != operator. If
  // they are not equal the test passes, otherwise the test fails.
  template<typename T1, typename T2>
  bool isnt (const T1& this_one,
             const T2& that_one,
             const std::string& test_name)
  {
    bool passed = (this_one != that_one);
      
    ok (passed, test_name);
      
    if (!passed && !skip_enabled_ && !todo_enabled_) {
      output_ << "#    '" << this_one << "'\n";
      output_ << "#      !=\n";
      output_ << "#    '" << that_one << "'\n";
    }
      
    return passed;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: pass
  //
  // Parameters:
  //    test_name - A short, descriptive name for this test
  //
  // Marks the given test as trivially passing.
  bool pass (const std::string& test_name) {
    return ok(true, test_name);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: fail
  //
  // Parameters:
  //    test_name - A short, descriptive name for this test
  //
  // Marks the given test as trivially failing.
  bool fail (const std::string& test_name) {
    return ok(false, test_name);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: enable_todo
  //
  // Enables todo mode causing no tests to be run until todo is disabled.
  // Always returns true.
  bool enable_todo () {
    todo_enabled_ = true;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: disable_todo
  //
  // Disables todo mode causing any further tests to be run. Always returns
  // false.
  bool disable_todo () {
    todo_enabled_ = false;
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: enable_skip
  //
  // Parameters:
  //   reason - The reason printed for skipping tests
  //
  // Enables skipping causing no tests to be run until skipping is disabled.
  // Always returns true.
  bool enable_skip () {
    skip_enabled_ = true;
    return true;
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Function: disable_skip
  //
  // Disables skipping allowing tests to be run again. Always returns false.
  bool disable_skip () {
    skip_enabled_ = false;
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: num_failed
  // 
  // Returns the number of failed tests
  unsigned int num_failed () const {
    return num_failed_;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Function: num_skipped
  //
  // Returns the number of skipped tests
  unsigned int num_skipped() const {
    return num_skipped_;
  }
  private:
  unsigned int    num_tests_; // The total number of tests to be executed
  unsigned int    test_number_; // The number of the current test
  unsigned int    num_skipped_; // The number of tests marked as skipped
  unsigned int    num_failed_; // The number of tests marked as failing
  unsigned int    num_planned_; // The number of tests planned to be run
  bool            skip_enabled_; // Are tests being skipped
  bool            todo_enabled_; // Are these tests incomplete
  output_policy_t output_; // The place where output will be sent
  };
}

#endif // LEMON_H_
