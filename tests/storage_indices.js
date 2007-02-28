/* test for storage */

function DumpObj( obj )
{
  stdout << "dumping object: \n" ;

  for(var n in obj )
  {
    { stdout << "Property: '" << n << "'\tValue: typeof '" << typeof(obj[n]) << "' value '" << obj[n] << "'\n"; }
  }
}


function CreateAndFillDB( dbName )
{
	var s = Storage.open( dbName );

	var indexA = s.createIndex(#integer);
	var indexB = s.createIndex(#string);


	var key = 200;
  for( var i = 0; i < 10; i=i+1 )
	{
    key = (31415*key + 27182) % 20007;
    var rec = {};
    rec.key = key;
    rec.value = "Value: " + i.toString();
    indexA[rec.key] = rec;
stdout << "inserted (" << rec.key << "," << rec.value << ") into indexA\n";
    indexB[rec.value] = rec;
	}

	var root = {}; 
	root["indexA"] = indexA;
	root["indexB"] = indexB;
stdout << "inserted indexA and B into root\n";
	s.root = root;

	s.close();
stdout << "storage closed\n";
}


function main()
{
	var s = Storage.open( "indexed.db" );

	if( s.root == null )
	{ 
		s.close();
stdout << "CreateAndFillDB( 'indexed.db' )\n";
		CreateAndFillDB( "indexed.db" );
		s = Storage.open( "indexed.db" );
	}

	var root = s.root;
stdout << "root loaded\n";
	var index1 = root["indexA"];
	var index2 = root["indexB"];
stdout << "indexA and B are loaded from root\n";

	try
	{
stdout << "reading objs from indexA\n";
	  for(var res in index1)
	  {
		  stdout << "Key: " << res.key << " Value: " << res.value << "\n";
	  }
	  stdout << "end of results\n";
	}
	catch( e )
	{
		stdout << "Exception in accessing DB index\n" << e;
	}

	var searchRes = index1.select(1000, 7000);
stdout << "Searching (1000, 7000)\n";
stdout << "reading objs from search results\n";
	  for(var res in searchRes)
	  {
		  stdout << "Key: " << res.key << " Value: " << res.value << "\n";
	  }
	  stdout << "end of results\n";

	var searchRes2 = index2.select("Value: 5", "Value: 9");
stdout << "Searching ('Value: 5', 'Value: 9')\n";
stdout << "reading objs from STRING search results\n";
	  for(var res in searchRes2)
	  {
		  stdout << "Key: " << res.value << " Value: " << res.key << "\n";
	  }
	  stdout << "end of results\n";

stdout << "reading ALL objs from STRING index\n";
	  for(var res in index2)
	  {
		  stdout << "Key: " << res.value << " Value: " << res.key << "\n";
	  }
	  stdout << "end of results\n";

stdout << "reading ALL objs from THE SAME string index\n";
	  for(var res in index2)
	  {
		  stdout << "Key: " << res.value << " Value: " << res.key << "\n";
	  }
	  stdout << "end of results\n";

}


main();
gc();
