app.controller("dashboardController", function ($scope, $location,apiService) {
	console.log($location)
	
	$scope.merchantSales = {
		stages: ['Center Stage', 'South Stage', 'West Stage'],
		data: [47000, 9000, 45000],
		backgroundColor: ['#3152dc', '#6eb59f', '#df4f4f']
	}
	
	$scope.modalTitle = " ";
	$scope.modalContent = " ";

	document.getElementById('5').setAttribute('titleText','Vendor is confused!');
	document.getElementById('5').setAttribute('message','Why do you want them to move? They do not have anything to sell!');

	document.getElementById('30').setAttribute('titleText',' ');
	document.getElementById('30').setAttribute('message','This location is forecasted to generate 25% increase in sales. Keep it up!');

	document.getElementById('75').setAttribute('titleText','Great Job!');
	document.getElementById('75').setAttribute('style','white-space:pre')
	document.getElementById('75').setAttribute('message','Thanks to your swift action this <br/> location is forecasted to generate <br/> 50% increase in sales!');

	document.getElementById('95').setAttribute('titleText','Great Job!');
	document.getElementById('95').setAttribute('message','Thanks to your swift action this <br/> location is forecasted to generate <br/>  80% increase in sales!');
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
	
	
	$scope.gotoMain = function() {
		//$location.path("/main");
		document.getElementById("mySidenav").style.width = "47%";
		document.getElementById("dashboard-backdrop").classList.add('dashboard-backdrop');
	}
	
/* 	$scope.openNav = function() {
    	document.getElementById("mySidenav").style.width = "47%";
	} */

	$scope.closeNav = function() {
		document.getElementById("mySidenav").style.width = "0%";
		document.getElementById("dashboard-backdrop").classList.remove('dashboard-backdrop');

		//$scope.gotoMain();
	}
	
/* 	if(sideNav) {
		$scope.openNav();
		sideNav = false;
	} */

	if(moveCart){
		$('#moveCartModal').modal({
			keyboard : false,
			backdrop : 'static'
		});
		moveCart = false;
	}



	$scope.handleDrop = function(item,modalTitle,modalContent) {
		$scope.modalTitle = modalTitle;
		$scope.modalContent = modalContent;
		$('#movedCartModal').on('hidden.bs.modal', function () {
			$scope.$apply(function () {
				// console.log($location)
				$location.url('/main')
				// console.log($location)
			});
		})
		//$location.path('/main')
		
		$('#movedCartModal').modal({
			keyboard : false,
			backdrop : 'static'
		});

		setTimeout(function() {
			$('#movedCartModal').modal('hide');
		},10000);
		
	}
});


app.directive('draggable', function() {
	return {
        scope: {
            drop: '&',
            bin: '='
		},
		link:function(scope, element) {

			var el = element[0];
			el.draggable = true;
			el.addEventListener('dragstart',function(e) {
				e.dataTransfer.effectAllowed = 'move';
				e.dataTransfer.setData('Text', this.id);
				this.classList.add('drag');
				return false;
			},false);
			
			el.addEventListener('dragend', function(e) {
				this.classList.remove('drag');
				return false;
			},false);
			
			el.addEventListener('touchstart',function(e) {
				//console.log('in s')
				console.log(e)
				//var touch = event.targetTouches[0];
				var draggable_id = e.target.id;
				let draggable = document.getElementById(draggable_id);
				let style = getComputedStyle(draggable);
				scope.old_pos = {};
				scope.old_pos.pageX = style.left;
				scope.old_pos.pageY = style.top;
				console.log(scope.old_pos)
	/*			if(scope.old_target){
					draggable.style.left = scope.old_target.style.left;
					draggable.style.top = scope.old_target.style.top;
					scope.old_target.appendChild(draggable);
				}
				else{            
					draggable.style.left = touch.pageX-50 + 'px';
					draggable.style.top = touch.pageY-20 + 'px';
				}
	*/			return false;
			},false);
	
			el.addEventListener('touchmove',function(e) {
				//let wrapper = document.getElementById("drop-container");
				let draggable_id = e.target.id;
				draggable = document.getElementById(draggable_id);
				var touch = event.targetTouches[0];
				// Place element where the finger is
				draggable.style.left = touch.pageX-50 + 'px';
				draggable.style.top = touch.pageY-20 + 'px';
				e.preventDefault();
			},false);
			
			el.addEventListener('touchend',function(e) {
				let endTarget = document.elementFromPoint(
					e.changedTouches[0].pageX - 90,
					e.changedTouches[0].pageY + 20
				);
				let draggable_id = e.target.id;
				draggable = document.getElementById(draggable_id);
				let modalTitle;
				let modalContent;

					// call the passed drop function
					if(endTarget.id == 'drop-container') {
						//scope.old_target = endTarget.parentElement;
						//if(parseInt(draggable.id) < 50){
							//draggable.style.left = scope.old_pos.pageX 
							//draggable.style.top = scope.old_pos.pageY;
							//draggable.setAttribute("draggable", "false");
						//}	
						//else{
							draggable.classList.remove("cart"+draggable.id);
							document.getElementById('drop-container1').appendChild(draggable);
						//}
						// call the passed drop function
								
						//console.log(document.getElementById(draggable.id).getAttribute('title'))
						scope.$apply(function(scope) {
							var fn = scope.drop();
							modalTitle = document.getElementById(draggable.id).getAttribute('titleText');
							modalContent = document.getElementById(draggable.id).getAttribute('message');
							if ('undefined' !== typeof fn) {            
								fn(draggable.id,modalTitle,modalContent);
							}
						});
					}
					else if(endTarget.id == 'drop-container-south-stage'){
						draggable.classList.remove("cart"+draggable.id);
						document.getElementById('drop-container2').appendChild(draggable);

						if(draggable.id == 75 || draggable.id == 95){
							modalTitle = "Vendor is confused!";
							modalContent = "Why do you want them to move this full cart away from the crowd?"
						} 	
						else if(draggable.id == 5){
							modalTitle = document.getElementById(draggable.id).getAttribute('titleText');
							modalContent = document.getElementById(draggable.id).getAttribute('message');
						}
						
						if(draggable.id == 30){
							modalTitle = "Vendor is confused! ";
							modalContent = "Why do you want them to move? They do not have anything to sell!"
						} 
						scope.$apply(function(scope) {	
							var fn = scope.drop();
							if ('undefined' !== typeof fn) {            
								fn(draggable.id,modalTitle,modalContent);
							}
						});
					}
					else{
						/* if(scope.old_target){ */
							draggable.style.left = scope.old_pos.pageX;
							draggable.style.top = scope.old_pos.pageY;
							//scope.old_target.appendChild(draggable);
						/* } */
					}

				e.preventDefault();
				return false;
			},false);
	}}
    
});


app.directive('droppable', function() {
    return {
        scope: {
            drop: '&',
            bin: '='
        },
        link: function(scope, element) {
			var el = element[0];
            el.addEventListener('dragover',function(e) {
                e.dataTransfer.dropEffect = 'move';
                if (e.preventDefault) e.preventDefault();
                this.classList.add('over');
                return false;
                },false);
            
            el.addEventListener('dragenter',function(e) {
                this.classList.add('over');
                return false;
                },false);
            
            el.addEventListener('dragleave',function(e) {
                this.classList.remove('over');
                return false;
                }, false);
            
            el.addEventListener('drop', function(e) {
                if (e.stopPropagation) {
                    e.stopPropagation();
                    e.preventDefault();
                }
                
                this.classList.remove('over');
                
				var binId = this.id;
				var item = document.getElementById(e.dataTransfer.getData('Text'));
				console.log(binId);
				console.log(item.id)
				/* if(parseInt(item.id) < 50){
					item.setAttribute("draggable", "false");
				}	
				else{
					item.classList.remove("cart"+item.id);
					document.getElementById('drop-container1').appendChild(item);
				} */



				
                
                // call the passed drop function
              /*   scope.$apply(function(scope) {
                    var fn = scope.drop();
                    if ('undefined' !== typeof fn) {            
                        fn(item.id);
                    }
				}); */
				if(binId == 'drop-container') {
					item.classList.remove("cart"+item.id);
					document.getElementById('drop-container1').appendChild(item);
					//}
					// call the passed drop function
					
					//console.log(document.getElementById(draggable.id).getAttribute('title'))
					scope.$apply(function(scope) {
						var fn = scope.drop();
						modalTitle = document.getElementById(item.id).getAttribute('titleText');
						modalContent = document.getElementById(item.id).getAttribute('message');
						if ('undefined' !== typeof fn) {            
							fn(item.id,modalTitle,modalContent);
						}
					});
				}
				else if(binId == 'drop-container-south-stage' ){
					item.classList.remove("cart"+item.id);
					document.getElementById('drop-container2').appendChild(item);

					if(item.id == 75 || item.id == 95){
						modalTitle = "Vendor is confused!";
						modalContent = "Why do you want them to move this full cart away from the crowd?"
					} 	
					else if(item.id == 5){
						modalTitle = document.getElementById(item.id).getAttribute('titleText');
						modalContent = document.getElementById(item.id).getAttribute('message');
					}
					if(draggable.id == 30){
                                                modalTitle = "Vendor is confused! ";
                                                modalContent = "Why do you want them to move? They do not have anything to sell!"
                                        }

					scope.$apply(function(scope) {	
						var fn = scope.drop();
						if ('undefined' !== typeof fn) {            
							fn(item.id,modalTitle,modalContent);
						}
					});
				}
				
                return false;
                },
                false
            );
        }
    }
});
