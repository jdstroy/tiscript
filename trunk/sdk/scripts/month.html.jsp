<%
// create current month view
var date = new Date();
var caption = String.printf("%s %d", date.monthName(true), date.year);
var monthNo = date.month;
	date.day = 1;
	date.day = - (date.dayOfWeek - 1);
%>
<html>
<head>
<style> 
  th { font:10pt Arial; background-color:#EEE; border:1px solid #CCC; }
  td { font:10pt Arial; width:30px; text-align:center; border:1px solid #CCC; }
  td.othermonth { color:#CCC; }
</style>
</head>
<body>
<table border=1>
	<tr><th colspan=7><% =caption; %></th></tr>
	<tr><th>Sun</th><th>Mon</th><th>Tue</th><th>Wed</th><th>Thu</th><th>Fri</th><th>Sat</th></tr>
<% do { %>
	<tr><% for( var w=0; w < 7; ++w ) 
	      { 
	        %><td class="<% = monthNo!=date.month?"othermonth":"" %>"><% =date.day; %></td><% ++date.day; 
	      }%></tr>
<%    } while(date.month == monthNo); %>
</table>
</body>
</html>