var date = new Date();

date.month = 1;
date.day = 31;
date.hour = 0;
date.minute = 10;

for( var i = 0; i < 24; ++i )
{
  stdout.println( date.toLocaleString() );
  date.minute -= 1; 
}
