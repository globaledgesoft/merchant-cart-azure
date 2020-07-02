app.controller("mainController", function ($scope, $location, apiService) {
	const socket = io(window.location.origin);
	socket.on("stockUpdate", function(item) {
		$scope[item.name] = item.quantity;
		if(item.quantity % 2 == 0 && item.name == "beer" && item.quantity != 0){
			$('#beerAlertModal').modal({
				keyboard : false,
				backdrop : 'static'
			})
		}
	});
	
	socket.on("stockUpdate_restock", function(item) {
		$scope[item.name] = item.quantity;
	});

	$scope.merchantSales = {
		stages: ['Center Stage', 'South Stage', 'West Stage'],
		data: [47000, 9000, 45000],
		backgroundColor: ['#3152dc', '#6eb59f', '#df4f4f']
	}
	
        apiService.getInventoryItems(function(inventoryItems) {
		$scope.inventoryItems = inventoryItems;
		inventoryItems.map(function(item) {
			$scope[item.name] = item.quantity;
		});
	});
	

	inventoryLogData = [];
	inventoryLogLabel = [];

	apiService.getLogs(function(inventoryLogs){
		for(var index=0;index<inventoryLogs.data.length;index++){
			let inventoryData = inventoryLogs.data[index].quantity;
			let time = inventoryLogs.data[index].updatedtimelogs;
			let inventoryTime = new Date(moment.unix(time/1000));

			inventoryLogData.push(inventoryData);
			/* inventoryLevelsChart.data.datasets.forEach((dataset) => { */
				inventoryLogLabel.push(inventoryTime);
			/* }); */
			inventoryLevelsChart.update();	
		}
	})

	var inventoryLevelsCTX = document.getElementById("inventoryLevels").getContext('2d');
	var inventoryLevelsChart = new Chart(inventoryLevelsCTX, {
		type: 'line',
		data: {
			labels: inventoryLogLabel,
			datasets: [
			{
				label: 'Beers',
				fill:true,
				data: inventoryLogData,
				backgroundColor: '#7a9fff',
				borderColor: '#3152dc',
				borderWidth: 2,
				lineTension: 0,
				pointRadius: 0
			}]
		},
		options: {
			maintainAspectRatio: true,
			responsive: true,
			legend: {
				display: false
			},
			scales: {
				xAxes:[{
					ticks: {
						beginAtZero: true
					},
					type: 'time',
					format: "HH:mm",
               		time: {
						/* unit:'minute',
						unitStepSize: 30, */
						displayFormats: {
							'minute': 'HH:mm', 
							'hour': 'HH:mm', 
						}
               		},
				}],
				yAxes: [{
					gridLines:{
						borderDash: [8, 4],
						lineWidth:0,
					},
					ticks: {
//						stepSize: 10,
						beginAtZero:true
					},
					scaleLabel: {
						display: true,
						labelString: "# of Units",
						fontColor: "#495a75",
						fontSize: "14",
					}
				}],
			}
		}
	});
	
	var merchantSalesCTX = document.getElementById("merchantSales").getContext('2d');
	var merchantSalesChart = new Chart(merchantSalesCTX, {
		type: 'doughnut',
		data: {
			labels: $scope.merchantSales.stages,
			datasets: [
			{
				data: $scope.merchantSales.data,
				backgroundColor: $scope.merchantSales.backgroundColor,
				/* borderColor: '#FFFFFF', */
				/* borderWidth: 1, */
			}]
		},
		options: {
			elements: {
                arc: {
                    borderWidth: 0
                }
            },
			pieceLabel: {
                mode: 'value',
                render: function (args) {
                    return '$'+args.value;
                  },
                fontColor: function (args) {
                    return (args.dataset.backgroundColor[args.index]);
                  },
                position: 'outside',
                fontSize:14,
                fontStyle:'bold'
			},
			
			maintainAspectRatio: true,
			responsive: true,
			legend: {
				display: false,
			},
			cutoutPercentage: 40
		}
	});

	$scope.restock = function() {
		//$scope.inventoryItems.map(function(item){
			apiService.restockInventoryItem({name: "beer"});
		//});
	}
	
	$scope.gotoDashboard = function() {
		document.getElementById("mySidenav").style.width = "47%";
		document.getElementById("dashboard-backdrop").classList.add('dashboard-backdrop');
	}
	
	$scope.closeNav = function() {
		document.getElementById("mySidenav").style.width = "0%";
		document.getElementById("dashboard-backdrop").classList.remove('dashboard-backdrop');
	}

	let minTemp = 37.5, maxTemp = 39, range = 1.5, change = 0.1;
	$scope.tempF = parseFloat((minTemp + Math.random() * range).toFixed(1));
	$scope.tempC = parseFloat(((($scope.tempF - 32) * 5)/9).toFixed(2));
	
	setInterval(function(){
		$scope.$apply(function() {

                        apiService.getInventoryItems(function(inventoryItems) {
                          $scope.inventoryItems = inventoryItems;
                          inventoryItems.map(function(item) {
                          $scope[item.name] = item.quantity;
                           });
                        });

			if(Math.random() >= 0.5 && $scope.tempF < maxTemp) {
				$scope.tempF = parseFloat(($scope.tempF + change).toFixed(1));
			} else if($scope.tempF > minTemp){
				$scope.tempF = parseFloat(($scope.tempF - change).toFixed(1));
			}

			$scope.tempC = parseFloat(((($scope.tempF - 32) * 5)/9).toFixed(2));
		});
	}, 1000);

	$scope.gotoDashboard_moveCart = function(sidenavFlag){
		$('#beerAlertModal').modal('hide');
	}

	$('#beerAlertModal').on('hidden.bs.modal', function () {
		$location.path("/dashboard");
		moveCart = true;
	})
});
