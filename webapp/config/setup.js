var async = require('async');
//var sqlite3 = require('sqlite3').verbose();
//var db = new sqlite3.Database(config.knex.connection.filename);

var createInventoryTable = function(callback){
    knex.schema.createTable('inventory', function(table) {
        table.increments('id').primary();
        table.string('name', 50).unique().notNullable();
        table.integer('quantity', 50).notNullable();
    })
    .asCallback(function(err, rows) {
        if(!err) {
            callback(null, "Inventory table created");
        } else {
            callback(null, "Inventory table already exists");
        }
    });
};

var addInventoryItem1 = function(callback) {
	knex.insert(config.defaultItems["0"]).table('inventory')
    .asCallback(function(err, rows) {
        if (!err) {
            callback(null, "Successfully to added "+config.defaultItems["0"].name+" to inventory");
        } else {
			callback(null, config.defaultItems["0"].name+" already exists in inventory");
        }
    });
}

var addInventoryItem2 = function(callback) {
	knex.insert(config.defaultItems["1"]).table('inventory')
    .asCallback(function(err, rows) {
        if (!err) {
            callback(null, "Successfully to added "+config.defaultItems["1"].name+" to inventory");
        } else {
			callback(null, config.defaultItems["1"].name+" already exists in inventory");
        }
    });
}

var addInventoryItem3 = function(callback) {
	knex.insert(config.defaultItems["2"]).table('inventory')
    .asCallback(function(err, rows) {
        if (!err) {
            callback(null, "Successfully to added "+config.defaultItems["2"].name+" to inventory");
        } else {
			callback(null, config.defaultItems["2"].name+" already exists in inventory");
        }
    });
}

var createInventoryLogsTable = function(callback){
    knex.schema.createTable('inventorylogs', function(table) {
        table.increments('id').primary();
        table.integer('quantity').notNullable();
        table.bigint('updatedtimelogs').notNullable();
    })
    .asCallback(function(err, rows) {
        if(!err) {
            callback(null, "InventoryLogs table created");
        } else {
            callback(null, "InventoryLogs table already exists");
        }
    });
};

module.exports = {
	init : function(){
		async.series({
            createInventoryTable: createInventoryTable,
            createInventoryLogsTable : createInventoryLogsTable,
			addInventoryItem1: addInventoryItem1,
			addInventoryItem2: addInventoryItem2,
			addInventoryItem3: addInventoryItem3
		}, function(err, results){
			logger.debug(results);
		});
	}
}
