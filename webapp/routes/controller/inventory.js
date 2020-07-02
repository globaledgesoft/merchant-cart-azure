var moment = require('moment');

var inventoryImpl = require('../../services/db_services/inventoryImpl');
var inventoryImplObj = new inventoryImpl();

var inventory = function() {};

module.exports = inventory;

inventory.prototype.getItems = function(req, res) {
	let responseObject = {
		status : true,
		responseCode : 200,
		data : {}
	};

	inventoryImplObj.getItems(function(err, inventoryItems) {
		if(!err) {
			responseObject.data.inventoryItems = inventoryItems;
			responseObject.message = "Successfully fetched inventory items";
			res.json(responseObject);
		} else {
			responseObject.message = "Unable to get inventory items";
			responseObject.status = false;
			res.json(responseObject);
		}
	});
}

inventory.prototype.updateItem = function(req, res) {
	//let name = req.body.name;
	let name = "beer";
	//let quantity = req.body.quantity;
	
	let responseObject = {
		status : true,
		responseCode : 200,
		data : {}
	};
	
	let item = {
		name: name,
		//quantity: quantity
	}
	
	let updatedLogs = {
		//quantity : 0,
		updatedtimelogs : moment().local().valueOf()
	}

	inventoryImplObj.getItems(function(err, inventoryItems) {
		//console.log(inventoryItems)
		if(!err) {
			let itemIndex = inventoryItems.findIndex((item) => {return item.name == name});
			item.quantity = inventoryItems[itemIndex].quantity;
			item.quantity && (item.quantity--);
			//console.log(item.quantity)
			updatedLogs['quantity'] = item.quantity;
			
			inventoryImplObj.updateItem(item, function(err, data) {
				if(!err) {
					inventoryImplObj.updateLogs(updatedLogs,function(err,logs){
						if(!err){
							io.emit('stockUpdate', item);
							responseObject.message = "Successfully updated inventory item";
							console.log("inventoryLogs updated successfully")
							res.json(responseObject);
						}	
						else{
							console.log(err)
							responseObject.message = "Unable to update inventory logs";
							responseObject.status = false;
							res.json(responseObject);
						}
					});
				} else {
					responseObject.message = "Unable to update inventory item";
					responseObject.status = false;
					res.json(responseObject);
				}
			});
		} else {
			responseObject.message = "Unable to get inventory items";
			responseObject.status = false;
			res.json(responseObject);
		}
	});



}

inventory.prototype.restockItem = function(req, res) {
	//let itemId = req.body.id;
	let name = req.body.name;
	//let name = "beer";
	
	let responseObject = {
		status : true,
		responseCode : 200,
		data : {}
	};
	
	let item = {
		name: name
	}

	let updatedLogs = {
		//quantity : 220,
		updatedtimelogs : moment().local().valueOf()
	}


	let itemIndex = config.defaultItems.findIndex((item) => {return item.name == name});
	item.quantity = config.defaultItems[itemIndex].quantity;
	
	inventoryImplObj.restockItem(item, function(err, data) {
		if(!err) {
			updatedLogs['quantity'] = item.quantity;
			
			inventoryImplObj.updateLogs(updatedLogs,function(err,logs){
				if(!err){
					console.log("inventoryLogs updated successfully")
					io.emit('stockUpdate_restock', item);
					responseObject.message = "Successfully updated inventory item";
					res.json(responseObject);
				}	
				else{
					console.log(err)
					responseObject.message = "Unable to update inventory logs";
					responseObject.status = false;
					res.json(responseObject);
				}
			});

		} else {
			responseObject.message = "Unable to update inventory item";
			responseObject.status = false;
			res.json(responseObject);
		}
	});


}

inventory.prototype.getLogs = function(req, res) {
	let responseObject = {
		status : true,
		responseCode : 200,
		data : {}
	};
	
	inventoryImplObj.getLogs(function(err, data) {
		if(!err) {
			responseObject.message = "Successfully fetched inventoryLogs";
			responseObject.data = data;
			res.json(responseObject);
		} else {
			responseObject.message = "Unable to fetch inventoryLogs";
			responseObject.status = false;
			res.json(responseObject);
		}
	});
}
