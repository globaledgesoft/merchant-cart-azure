var inventoryImpl = function() {};

module.exports = inventoryImpl;

inventoryImpl.prototype.getItems = function(callback) {
	knex.select('id', 'name', 'quantity').table('inventory')
    .asCallback(function(err, rows) {
        if (!err) {
            callback(null, rows);
        } else {
            callback (err, null);
        }
    });
}

inventoryImpl.prototype.updateItem = function(item, callback) {
	knex.update(item).table('inventory').where('name', item.name)
    .asCallback(function(err, row) {
        if (!err) {
            callback(null, row);
        } else {
			logger.error("Unable to update inventory");
            callback (err, null);
        }
    });
}

inventoryImpl.prototype.restockItem = function(item, callback) {
	knex.update(item).table('inventory').where('name', item.name)
    .asCallback(function(err, row) {
        if (!err) {
            callback(null, row);
        } else {
			logger.error("Unable to restock item to inventory");
            callback (err, null);
        }
    });
}

inventoryImpl.prototype.getLogs = function(callback) {
	knex.select('id','updatedtimelogs','quantity').table('inventorylogs').where('updatedtimelogs','>=',21600000)
    .asCallback(function(err, row) {
        if (!err) {
            callback(null, row);
        } else {
			logger.error("unable to fetch inventoryLogs");
            callback (err, null);
        }
    });
}

inventoryImpl.prototype.updateLogs = function(updatedlogs,callback) {
	knex.insert(updatedlogs).table('inventorylogs')
    .asCallback(function(err, row) {
        if (!err) {
            callback(null, row);
        } else {
			logger.error("unable to update inventoryLogs");
            callback (err, null);
        }
    });
}