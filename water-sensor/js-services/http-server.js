const express = require('express');
const app = express();
const fs = require('fs');

let rawdata = fs.readFileSync('db.json');
let db = JSON.parse(rawdata);

app.use(express.json());

app.get('/', function (req, res) {
	res.send(db);
});

app.post('/', function (req, res) {
	db = req.body
	let data = JSON.stringify(db);
	console.log(data);
	fs.writeFileSync('db.json', data);
	res.send("okay");
});

app.listen(3000, function () {
  console.log('Example app listening on port 3000!');
});
