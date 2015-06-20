var app = angular.module('befunge', ['ngMaterial']);

app.config(function($mdThemingProvider) {
	$mdThemingProvider.theme('default')
		.primaryPalette('green')
		.accentPalette('yellow');
});

app.controller('AppCtrl', ['$scope', function($scope){

}]);
