var path = require('path');
var express = require('express');
var body_parser = require("body-parser");
config = require('./config/config');
logger = console;
var setup = require('./config/setup.js');
var routes = require('./routes/apiRoutes');
var socket = require('socket.io');
var app = express();

knex = require('knex')(config.knex);
setup.init();

app.engine('html', require('ejs').renderFile);
app.set('view engine', 'html');
app.use(express.static(path.join(__dirname + '/webapps')));
app.use(body_parser.json())

app.use(function(req, res, next) {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
    res.header("Access-Control-Allow-Methods", "GET, POST");
    next();
});

app.use('/', routes.router);

var server = app.listen(config.port, function () {
	var host = server.address().address;
	var port = server.address().port;
	logger.info("Application listening at http://%s:%s", host, port);
});

io = socket(server);

//var io_aws = require('socket.io-client')
//socket_aws = io_aws.connect('http://54.153.108.137:8655',{reconnect:true});
//socket_aws = socket('http://localhost:8000');

/* socket_aws.on("stockUpdate", function(item) {
    io.emit('stockUpdate', item);
}); */

module.exports = app;

/* let simulator = require("./simulator.js");
simulator(); */