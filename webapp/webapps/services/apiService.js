app.service('apiService', function($http) {
    this.getInventoryItems = function(cb) {
		$http.get('/api/getItems').then(
			(res) => {
				cb(res.data.data.inventoryItems)
			},
			(err) => {
				//alert('Unable to get inventory details')
				cb([]);
			}
		);
    }
	
	this.restockInventoryItem = function(item, cb) {
		$http.post('/api/restockItem', item).then(
			(res) => {
				console.log(res.data.message);
			},
			(err) => {
				console.log(err)
			}
		);
		}
		
		this.getLogs = function(cb) {
			$http.get('/api/getInventoryLogs').then(
				(res) => {
					cb(res.data)
				},
				(err) => {
					//console.log(err);
					cb([]);
				}
			);
			}
});
