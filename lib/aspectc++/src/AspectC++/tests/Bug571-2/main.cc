// Testing passing of "this" as argument of the call-wrapper if
// "that" pointcut functions with runtime checks are used.
// Additionally, this test checks if the runtime condition code
// accesses "this"/"that" via the TJP-Struct in case of multiple
// around advice."
#include <iostream>
using namespace std;
  
void func(const char* c) {
  cout << c << endl;
}
class A{
  public:
    virtual void memberfunc() {
      func("in A");
    }
};
class B : public A{
  public:
    virtual void memberfunc() {
      func("in B");
    }
};
class C : public A{
  public:
    virtual void memberfunc() {
      func("in C");
    }
};

int main() {
  B b;
  A& a = b;
  a.memberfunc();
  C c;
  A& a2 = c;
  a2.memberfunc();
  return 0;
}

const char* call_text = " call of \"func\" in B";
const char* execution_text = " execution of \"memberfunc\" in B";
aspect Asp {
  pointcut call_func_that_B() = call("void func(...)") && that("B");
  pointcut execution_memberfunc_that_B() = execution("void ...::memberfunc(...)") && that("B");
  
  advice call_func_that_B() : before() {
    cout << "before" << call_text << endl;
  }
  
  advice call_func_that_B() : after() {
    cout << "after" << call_text << endl;
  }
  
  advice call_func_that_B() : around() {
    cout << "around 1" << call_text << endl;
    tjp->proceed();
  }
  
  advice call_func_that_B() : around() {
    cout << "around 2" << call_text << endl;
    tjp->proceed();
  }
  
  
  advice execution_memberfunc_that_B() : before() {
    cout << "before" << execution_text << endl;
  }
  
  advice execution_memberfunc_that_B() : after() {
    cout << "after" << execution_text << endl;
  }
  
   advice execution_memberfunc_that_B() : around() {
    cout << "around 1" << execution_text << endl;
    tjp->proceed();
  }
  
  advice execution_memberfunc_that_B() : around() {
    cout << "around 2" << execution_text << endl;
    tjp->proceed();
  }
};

