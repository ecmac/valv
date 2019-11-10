var WebSocketServer = require('ws').Server;
wss = new WebSocketServer({ port: 8080, path: '/updates' });

wss.on('connection', function connection(ws) {
	console.log("connection");
    ws.on('message', function(message) {
		console.log("a: " + message);
		wss.broadcast(message);
    })
})

wss.broadcast = function broadcast(msg) {
   wss.clients.forEach(function each(client) {
       client.send(msg);
    });
};
