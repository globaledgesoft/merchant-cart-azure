var router = require('express').Router();

var inventory = require('../routes/controller/inventory');
var inventoryObj = new inventory();

// sending the main loading page
router.get('/', function(req, res){
    res.render('index');
});

// Inventory API
router.get('/api/getItems', inventoryObj.getItems);
router.get('/api/updateItem', inventoryObj.updateItem);
router.post('/api/restockItem', inventoryObj.restockItem);
router.get('/api/getInventoryLogs',inventoryObj.getLogs);

// export the routes to our application
module.exports = {
    router: router
};