
//|
//| String test
//|

function String_test()
{

  var s0 = "hello";
  var s1 = "world";
  var s2 = s0.concat(" ",s1);
  
  if( s2 != "hello world" ) throw "concat";
  
  if( s0.indexOf("l") != 2) throw "indexOf 1";
  if( s0.indexOf("h") != 0) throw "indexOf 2";
  if( s0.indexOf("h",1) >= 0) throw "indexOf 3";
  
  if( s0.lastIndexOf("l") != 3) throw "lastIndexOf";
  if( s0.lastIndexOf("o") != 4) throw "lastIndexOf";
  
  if( s0.charAt(2) != "l" ) throw "charAt";
  if( s0.charCodeAt(2) != 0x6C ) throw "charCodeAt";
  
  if( String.fromCharCode(112, 108, 97, 105, 110) != "plain") throw "fromCharCode";
  
  // range test
  var s3 = "hello world";
  if( s3[2..5] != "llo" ) throw "range 1";
  if( s3[..5] != "hello" ) throw "range 2";
  if( s3[6..] != "world" ) throw "range 3";
  
  var r, re1;         
  var s = "The rain in Spain falls mainly in the plain";
  re1 = /ain/i;     //Create regular expression pattern.
  r = s.match(re1);  //Attempt match on search string.
  if( r != "ain" ) throw "match/i";
  
  var re = /ain/ig;      
  r = s.match(re);   
  if( r.length != 4 ) throw "match/gi";
 
  if( s.replace(re,"1") != "The r1 in Sp1 falls m1ly in the pl1" )  throw "replace/gi";
  
  
  if( s.search( "Spain" ) != 12 ) throw "search 1";
  if( s.search( "Russia" ) != -1 ) throw "search 2";
  
  if( s.slice( -5 ) != "plain" ) throw "slice 1";
  if( s.slice( 12, 17 ) != "Spain" ) throw "slice 2";
  if( s.slice( 3 ) != " rain in Spain falls mainly in the plain" ) throw "slice 3";
  
  if( s.split( " " ).length != 9 ) throw "split1";
  if( s.split( re1 ).length != 5 ) throw "split2";
    
  if(s.substr(12, 5) != "Spain") throw "substr";
  if(s.substring(12, 17) != "Spain") throw "substring";

  if("This is a STRING object".toLowerCase() != "this is a string object") throw "toLowerCase"; 
  if("This is a STRING object".toUpperCase() != "THIS IS A STRING OBJECT") throw "toUpperCase"; 
  
  if("This is a STRING object".toString() != "This is a STRING object") throw "toString"; 
  if("This is a STRING object".valueOf() != "This is a STRING object") throw "valueOf"; 
  
  if( ("Hello" + (1).toString()) != "Hello1") throw "join"; 
   
}

function Array_test()
{
  var a0 = [1,2,3,4,5,6,7,8,9,10];
  var a1 = new Array(1,2,3,4,5,6,7,8,9,10);
  
  stdout.printf("%v\n", a1);
  
  var x1 = [2,3,4];
  var x2 = [1,2,3];
  var x3 = [8,9,10];
  
  // range test
  if( a0[1..4] != x1 ) throw "range 1";
  if( a0[..3] != x2 ) throw "range 2";
  if( a0[7..] != x3 ) throw "range 3";
  
  if(a0.length != 10) throw "ctor1a, length=" + a0.length; 
  if(a0[9] != 10) throw "ctor1b"; 
  if(a0[0] != 1) throw "ctor1c"; 
  if(a1.length != 10) throw ("ctor2a," + a1); 
  if(a1[9] != 10) throw "ctor2b"; 
  if(a1[0] != 1) throw "ctor2c"; 
  
  if(a0.first != 1) throw "first get"; 
  if(a0.last != 10) throw "last get"; 
  
  if(a0.pop() != 10) throw "pop1"; if(a0.last != 9) throw "pop2"; 
  if(a0.shift() != 1) throw "shift1"; if(a0.first != 2) throw "shift2"; 
  
  a0.push(10); if(a0.last != 10) throw "push"; 
  a0.unshift(1); if(a0.first != 1) throw "unshift"; 
  if(a0.length != 10) throw "push/pop"; 
  
  var a2 = [0,1,2];
  a2 = a2.concat(3,4,5);
  if(a2.length != 6) throw "concat1"; 
  if(a2.first != 0 || a2.last != 5 ) throw "concat2"; 
  
  if(a2.join("-") != "0-1-2-3-4-5" ) throw "join";
  
  a2.reverse(); 
  if(a2.first != 5 || a2.last != 0 ) throw "reverse"; 
  
  var a3 = [2,3,4];
  if( a0.slice( 1,4 ) != a3 ) throw "slice";
  
  var a4 = a1.splice(1,3); 
  if( a4 != a3 ) throw "splice1";
  if( a1.length != 7 ) throw "splice2";
  if( a1[1] != 5 ) throw "splice3";
  if( a1.last != 10 ) throw "splice4";
  
  var a5 = [10,9,8,7,6,5,4,3,2,1];
  var a6 = a5.clone();
  var a7 = a5.clone();
  a5.sort();
  if( a5 != a0 ) throw "sort1";
  
  function more(v1,v2)
  {
    return v2 - v1;
  } 
  a5.sort(more);
  if( a5 != a6 ) throw "sort2";


  a7.sort(:v1,v2:v1-v2);
  if( a7 != a0 ) throw String.printf("sort3 with lambda, arrays are %v %v\n", a7, a0);
    
    
  if( a0.toLocaleString() != a0.toString() ) throw "toLocaleString | toString";
  
  var sum = 0;
  for( var el in [10,20,30] ) sum += el;
  if( sum != 60 ) throw "for(var in array)";
 
}

function Integer_test()
{
  var imax = Integer.MAX;
  var imin = Integer.MIN;
  if(imax != 2147483647) throw "Integer.MAX"; 
  if(imin != -2147483647-1) throw "Integer.MIN"; 
  
  if("314".toInteger() != 314 ) throw "toInteger"; 
  if("ABC".toInteger() != undefined ) throw "toInteger not integer string"; 
  if("ABC".toInteger(314) != 314 ) throw "toInteger not integer string, default value"; 
  
  if(2006 % 100 != 6 ) throw "remainder!"; 
  
  if(Integer.max(3,1,4) != 4 ) throw "Integer.max"; 
  if(Integer.min(3,1,4) != 1 ) throw "Integer.min"; 

  if(Integer.max(3,1,4,[5,6,7]) != 7 ) throw "Integer.max sub array"; 
  if(Integer.min(3,1,4,[0,-1,-2]) != -2) throw "Integer.min sub array"; 
}

function Float_test()
{
  var fmax = Float.MAX;
  var fmin = Float.MIN;
 
  if(fmax.toString() != "1.#INF") throw "Float.MAX"; 
  if(fmin.toString() != "-1.#INF") throw "Float.MIN"; 
  
  if("3.14".toFloat() != 3.14 ) throw "toFloat"; 
  if("ABC".toFloat() != undefined ) throw "toFloat not float string"; 
  if("ABC".toFloat(3.14) != 3.14 ) throw "toFloat not float string, default value"; 
  
  if(Float.max(3.0,1.0,4.0) != 4.0 ) throw "Float.max"; 
  if(Float.min(3.0,1.0,4.0) != 1.0 ) throw "Float.min"; 

  if(Float.max(3.0,1.0,4.0,[5.0,6.0,7.0]) != 7.0 ) throw "Float.max sub array"; 
  if(Float.min(3.0,1.0,4.0,[5.0,6.0,7.0]) != 1.0 ) throw "Float.min sub array"; 
}




type Foo  
{
  function this(n = "o1")   { this.one = 1; this._name = n; }
  function foo()            { return "foo"; }
  property name(v)                
          { 
            get { return "foo:" + this._name; } //get part
            set { this._name = v; } // set part
          }
}

type Bar: Foo // type 'Bar' is derived from 'Foo'
{
  function this(n = "o2")   { super(n); this.two = 2; }
  function bar()            { return "bar"; }
  function foo()            { return super.foo() + "bar"; }
  property name(v)          { get return "bar:" + this._name; set this._name = v; }
}


function Object_test()
{

// classic class use cases

  var foo = new Foo();  
  if(foo.one != 1 ) throw "class ctor 1";
  if(foo.foo() != "foo" ) throw "class method 1";
  if(foo.prototype !== Foo) throw "prototype/get 1";
  if(foo.name != "foo:o1") throw "property/get 1";
  
  foo.name = "O1";
  if(foo.name != "foo:O1") throw "property/set 1";
  
  
  var bar = new Bar(); // Bar is derived from Foo 
  if(bar.one != 1 ) throw "class ctor 2.1";
  if(bar.two != 2 ) throw "class ctor 2.2";
  if(bar.bar() != "bar" ) throw "class method 2.1";
  if(bar.foo() != "foobar" ) throw "class method 2.2";  
  if(bar.prototype !== Bar) throw "prototype/get 2";
  if(bar.name != "bar:o2") throw "property/get 2";
  
  bar.name = "O2";
  if(bar.name != "bar:O2") throw "property/set 2";
  
  if( typeof bar != #object ) throw "typeof";
  if( !(bar instanceof Bar) ) throw "instanceof 1";
  if( !(bar instanceof Foo) ) throw "instanceof 2";


// class literals and dynamic subclassing

  var o = { one:1, two:2, "three":3 };
  if(o.toString() != "[object Object]" ) throw "toString";
  if(o.one != 1 ) throw "literal 1";
  if(o["three"] != 3 ) throw "literal 2";
  
  
  o.prototype = Foo;  // assign class to existing object instance.
                        // yep, it works in the script.
  if(o.foo() != "foo" || o.one != 1 ) throw "dynamic class change";
  
  var oc = o.clone(); // does copy!
  if( oc === o || oc.foo() != "foo" || oc.one != 1 ) throw "clone";
  
  if( !("one" in oc) ) throw "in";
  
  // non JS feature: literals of user defined classes
  var o2 = {:Foo one:1 }; // this is actually short form of var o2 = {prototype:Class, one:1 };
  if(o2.one != 1 ) throw "literal class 1 ";
  if(o2.foo() != "foo" ) throw "literal class 2";
  
  // non JS feature: get element by literal 
  var o3 = { one:1, two:2, three:3, #thirty-three: 33 };
  if(o3[#one] != 1 ) throw "attribute by symbol 1 ";
  // short form of the line above
  if( o3#one != 1 ) throw "attribute by symbol 2 ";
  if( o3#thirty-three != 33 ) throw "attribute by symbol 3 ";
  
 
}

const MY_CONST = 12;

function Const_test() 
{
  if( MY_CONST != 12 ) throw "const 1";
  var got_error = false;
  try { MY_CONST = 13; } catch(e) { got_error = true; }
  if( !got_error ) throw "const 2";

  const MY_LOCAL_CONST = 24;
  if( MY_LOCAL_CONST != 24 ) throw "const 3";
  
  //next line shall produce compile time error.
  //MY_LOCAL_CONST = 25;
  
}


function Date_test() 
{
  var d = new Date();
  stdout.printf("today is:%d-%d-%d %d:%d:%d\n", d.day, d.month, d.year,
      d.hour, d.minute, d.second );
  
  d.day = 28;
  
  stdout.printf("today is:%d-%d-%d %d:%d:%d\n", d.day, d.month, d.year,
      d.hour, d.minute, d.second );
      
  d = new Date("28 Feb 1980 UTC");
  
  if(   d.UTCday != 28 || 
        d.UTCmonth != 2 || 
        d.UTCyear != 1980 ) throw "date ctor";
 
  stdout.printf("date is:%d-%d-%d %d:%d:%d\n", 
      d.day, d.month, d.year,
      d.hour, d.minute, d.second );
      
  stdout.printf("locale format short:%s\nlocale format long:%s\n", d.toLocaleString(), d.toLocaleString(true));
  
  var n = d.valueOf();
  n += 24 * 60 * 60 * 1000.0;
  d = new Date(n);
      
  stdout.printf("date is:%d-%d-%d %d:%d:%d\n", d.day, d.month+1, d.year,
      d.hour, d.minute, d.second );
  
}


function Function_test()
{
  // var o = 1;
  // arg with the default value test
  
  function DefParams(test = 314)
  {
    return test;
  }
 
  if(DefParams() != 314) throw "Function, default parameters values"; 
  
  // varargs
  function VarArgs(args..)
  {
    //stdout << "args are:" << args;
    //stdout << " typeof args is:" << typeof args << "\n";
    return args.length;
  }
  if(VarArgs(1,2,3) != 3) throw "Function, varargs!"; 

  // varargs, rest
  function VarArgs2(first, restOfArgs..)
  {
    return restOfArgs;
  }
  
  if(VarArgs2(1,2,3) != [2,3]) throw "Function, varargs 2!"; 

  var obj = { one:1, two:2 };
  
  // apply test #1
  function ExtFunc()
  {
    return this.one + this.two;
  }
  if( ExtFunc.apply(obj) != 3) throw "Function, apply (0) failed !"; 
  if( ExtFunc.call(obj) != 3) throw "Function, call (0) failed !"; 

  // apply test #2, additional parameters
  function ExtFunc1(three,four)
  {
    return this.one + this.two + three + four;
  }
  if( ExtFunc1.apply(obj,[3,4]) != 10) throw "Function, apply (1) failed !"; 
  if( ExtFunc1.call(obj,3,4) != 10) throw "Function, call (1) failed !"; 
  
  
  // lambda tests
  
  // simplest form 
  

  var lambda1 = :a,b: a+b;
  
  if( typeof lambda1 != #function) throw "Lambda 1.1 failed !"; 
  if( lambda1(3,4) != 7) throw "Lambda 1.2 failed !"; 
  
  // second form of lambda declaration - block with parameters
  var lambda2 = :a,b { return a+b; }
  if( typeof lambda2 != #function) throw "Lambda 2.1 failed !"; 
  if( lambda2(3,4) != 7) throw "Lambda 2.2 failed !"; 
  
  // third, classic lambda declaration
  var lambda3 = function (a,b) { return a+b; }
  if( typeof lambda3 != #function) throw "Lambda 3.1 failed !"; 
  if( lambda3(3,4) != 7) throw "Lambda 3.2 failed !"; 
  
}



function Stream_test()
{
  var ss = Stream.openString(); // in memory string stream, a.k.a. stram builder
  ss << "one"; 
  ss << "two";
  ss << "three";
  ss.printf("%d%d%d", 1, 2, 3);
  
  if(ss.toString() != "onetwothree123") throw "String Stream!"; 
  
  //too noisy but works
  //var f = Stream.openFile( "tests.js", "r" );
  //if(!f) 
  //{
  //  stdout.printf("unable to open file tests.js\n");
  //  return;
  //}
  //while( true )
  //{
  //  var s = f.readln();
  //  if(s == undefined) break;
  //  stdout.println(s);
  //}
  //stdout.println("------------");
 
  var t = { one:1, two:2, three:3 };
  // print t in format suitable for 'eval'!
  stdout.printf("return %v;\n",t);
  
}

function SocketStream_test()
{
  // will print content of front page:
  var sock = Stream.openSocket( "www.terrainformatica.com:80" );
  
  sock.println("GET http://www.terrainformatica.com/main.whtm HTTP/1.0");
  sock.println("User-Agent: TiScript [en]");
  sock.println("");

  while( true )
  {
    var s = sock.readln();
    if(s == undefined) break;
    stdout.println(s);
  }
  stdout.println(".");
}

type NS // namespace
{
  var pi = 314;
  function foo() { return pi; }
  type NSS // namespace
  {
    function bar() { return NS.foo(); }
    function toz() { return bar(); }  // 'type' establishes distinct namespace.
                                      // so 'bar' is accessible here as a global function
  }
}

function Namespace_test()
{
  var t1 = NS.foo();  
  if( t1 != 314 ) throw "namespace 1";
  var t2 = NS.NSS.toz();  
  if( t2 != 314 ) throw "namespace 2";
}


function ExtLoop_test()
{
  var c = 0;
  while:No1 ( c++ < 100 )
  {
    for:No2 (var n = 0; n < 1; ++n) 
    {
      if( c >= 5 ) break No1;
      continue No1;
    }
  }
  if( c != 5 )  throw "named loop #" + c.toString();
}



function run(TestFunc)
{
  try {
    TestFunc();
    stdout.printf("Test '%s' passed\n", TestFunc );
  }
  catch(err)
  {
    stdout.printf("Test '%s' failed on '%s'!\n", TestFunc, err );
  }
}

run( String_test );
run( Array_test );
run( Const_test );
gc(); // test GC
run( Object_test );
run( Date_test );
run( Stream_test );
//This is too noisy, so I've commented it out for a while.
//run( SocketStream_test );
run( Integer_test );
run( Float_test );
run( Function_test );
run( Namespace_test );
run( ExtLoop_test )



