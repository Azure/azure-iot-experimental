var connect = require('connect');
var serveStatic = require('serve-static');

var app = connect();

app.use(function (req, res, next) {
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, PUT, OPTIONS, HEAD");
    next();
});

app.use(serveStatic(__dirname + '/web-content'))
app.listen(8080, function(){
    console.log('Server running on 8080...');
});