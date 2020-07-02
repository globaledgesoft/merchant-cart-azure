var request = require('request');
var socket = require('socket.io-client');
let beerOutTime = 3000
const serverUrl = "http://localhost:8655";
const apiUrl = "http://localhost:8655/api"
var beerQty = 220;
var koozieQty = 103;
var openerQty = 9;

module.exports = function() {
	
	getItems(function(inventoryItems){
		//console.log(inventoryItems)
		inventoryItems.map(function(item) {
			switch(item.name) {
				case "beer"    : beerQty = item.quantity;
								 break;
				case "koozies" : koozieQty = item.quantity;
								 break;
				case "openers" : openerQty = item.quantity;
								 break;
			}
		});

		setInterval(function(){
			beerQty && beerQty--;
			decrementItem()
		}, beerOutTime);
	});
}

function getItems(cb) {
	let options = {
		url: apiUrl+'/getItems',
		method: 'GET',
		headers: {
			'Content-Type': 'application/json'
		}
	};
	
	request(options, function(err, res, body) {
		body = JSON.parse(body)
		
		if(body.status) {
			cb(body.data.inventoryItems);
		} else {
			cb([]);
		}
	});
}

function decrementItem() {
/* 	let data = {
		name: name,
		quantity: quantity
	}; */
	
	let options = {
		url: apiUrl+'/updateItem',
		method: 'GET',
		headers: {
			'Content-Type': 'application/json'
		},
	};
	
	request(options, function(err, res, body) {
		body = JSON.parse(body)
		
		if(body.status) {
			//console.log(body.message);
		} else {
			//console.log(body.message);	
		}
	});
}