<%
var months = [ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
%>
<html>
<body>
<table>
  <tr><th>No</th><th>Name</th></tr>
<% for(var i = 0; i < 12; ++i ) { %>
  <tr><td><% =i+1 %></td><td><% =months[i] %></td></tr>
<% } %>
</table>
</body>
</html>