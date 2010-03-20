///////////////////////////////////////////////////////////////////////////////
// lemon.h - Contains the entire lemon unit testing framework
//
// Time-stamp: <Last modified 2010-03-17 09:59:15 by Eric Scrivner>
//
// Description:
//   A lightweight, minimal unit-testing framework based on Perl Test::More
//
// Copyright (c) 2010 Lemon Team
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
#include <string>
#include <iostream>

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
  // Policy-based class for doing testing
  template <class output_policy_t = lemon::output::cout>


  class test {
  public:
    ///////////////////////////////////////////////////////////////////////////
    // Function: test
    //
    // Parameters:
    //    num_planned_tests - The total number of tests you plan to execute
    //
    // This simply lets lemon know how many tests you're planning to run so that
    // it can properly output the diagnostic information and doesn't have to
    // count by hand (which can be tricky as one test can have many assertions).
    test (unsigned int num_planned_tests)
    : num_tests_(0),
      test_number_(0),
      num_skipped_(0),
      num_failed_(0),
      num_planned_(num_planned_tests)
    {
      output_ << "1.." << num_planned_tests << "\n";
    }
  
    ///////////////////////////////////////////////////////////////////////////
    // Function: done
    //
    // Signifies the end of the testing phase and prints the results.
    //
    // Returns true if all unskipped tests passed, false if there were failures.
    bool done () {
      // If any tests were skipped
      if (num_skipped_ > 0) {
        // Display information about the skipped tests
        output_ << "# Looks like you planned " << num_tests_;
        output_ << " but only ran " << (num_tests_ - num_skipped_) << "\n";
      }
    
      // If any tests were failed
      if (num_failed_ > 0) {
        // Display test failure statistics
        output_ << "# Looks like you failed " << num_failed_;
        output_ << " of " << num_tests_ << "\n";
        return false;
      } else if(num_tests_ > num_planned_) {
        output_ << "# Looks like you ran " << num_tests_ << " tests, ";
        output_ << "but only planned " << num_planned_ << "\n";
        return false;
      } else {
        // Otherwise display success message
        output_ << "# Looks like you passed all " << num_tests_ << " tests.\n";
        return true;
      }
    }
  
    ///////////////////////////////////////////////////////////////////////////
    // Function: diag
    //
    // Parameters:
    //    message - A string to be written out to the display
    //
    // Used to display diagnostic information which is not a unit test.
    void diag (const std::string& message) {
      output_ << "# " << message << "\n";
    }
  
    ///////////////////////////////////////////////////////////////////////////
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
    
      // If the test was passed
      if (passed) {
        // Inform you that the test passed
        output_ <<"ok " << num_tests_ << " " << test_name_out << "\n";
      } else {
        // Otherwise increment the number of failed tests
        num_failed_++;
      
        // Inform you that the test failed
        output_ << "not ok " << num_tests_ << " " << test_name_out << "\n";
        diag("  Failed test '" + test_name + "'");
      }
  
      return passed;
    }
  
    ///////////////////////////////////////////////////////////////////////////
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

    ///////////////////////////////////////////////////////////////////////////
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
      
      if (!passed) {
        output_ << "#         got: '" << this_one << "'\n";
        output_ << "#    expected: '" << that_one << "'\n";
      }
      
      return passed;
    }

    ///////////////////////////////////////////////////////////////////////////
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
      
      if (!passed) {
        output_ << "#    '" << this_one << "'\n";
        output_ << "#      !=\n";
        output_ << "#    '" << that_one << "'\n";
      }
      
      return passed;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function: pass
    //
    // Parameters:
    //    test_name - A short, descriptive name for this test
    //
    // Marks the given test as trivially passing.
    bool pass (const std::string& test_name) {
      return ok(true, test_name);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function: fail
    //
    // Parameters:
    //    test_name - A short, descriptive name for this test
    //
    // Marks the given test as trivially failing.
    bool fail (const std::string& test_name) {
      return ok(false, test_name);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function: skip
    //
    // Parameters:
    //    reason - The reason for skipping this test
    //    num_to_skip - The number of tests to skip
    //
    // Skips the given number of tests adding them to the skip count.
    void skip (const std::string& reason, unsigned int num_to_skip) {
      num_skipped_ += num_to_skip;
  
      for (unsigned int i = 0; i < num_to_skip; i++) {
        pass("# SKIP " + reason);
      }	
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function: todo
    //
    // Parameters:
    //    what - What needs to be done
    //
    // Prints a message indicating what is left to be done
    void todo (const std::string& what) {
      num_skipped_++;
      pass("# TODO " + what);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function: num_failed
    // 
    // Returns the number of failed tests
    unsigned int num_failed () const {
      return num_failed_;
    }
  private:
    unsigned int    num_tests_; // The total number of tests to be executed
    unsigned int    test_number_; // The number of the current test
    unsigned int    num_skipped_; // The number of tests marked as skipped
    unsigned int    num_failed_; // The number of tests marked as failing
    unsigned int    num_planned_; // The number of tests planned to be run
    output_policy_t output_; // The place where output will be sent
  };
}

#endif // LEMON_H_
