/* test for storage gc() */

type MyClass
{
  function this(n = "some name")  { this._name = n; }
  property name(v)                { get return this._name; }
}

function ReadRoot()
{
  var storage_out = Storage.open("test.db");
  
  if(!storage_out.root)
  {
    storage_out.root = new MyClass("Hello world");
    stdout << "storage 'test.db' created\n";
  }
  else 
    stdout << "storage 'test.db' opened\n";
 
  return storage_out.root;
}

function main()
{
  var root = ReadRoot();
  stdout << "root.name " << root.name << "\n";
}

main();
stdout << "GC after main() \n";
gc();
