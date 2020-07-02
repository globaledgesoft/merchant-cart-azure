var app = angular.module('app', ['ngRoute','ngSanitize']);

var sideNav = false;
var moveCart = false;

app.config(function($routeProvider){
	$routeProvider
		.when('/main', {
			controller: 'mainController',
			templateUrl: 'views/main.html'
		})
        .when('/dashboard', {
			controller: 'dashboardController',
			templateUrl: 'views/dashboard.html'
		})
		.otherwise({redirectTo: '/main'});
});