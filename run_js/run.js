console.log(bmonmate.help());
var connector = bmonmate.create_daemon_connector();
console.log('\n' + connector)
var state = bmonmate.connect_to_daemon(connector, "dmx1.loc", 8050);
if (state < 0) {
 console.log("failed to connect");
}else {
var user = "msamsonov";
var passwrd = "7SqiiFUNWM5NrMirSHHh";
var reply = bmonmate.login(connector, user, passwrd);
if (reply < 0){
    console.log("failed to login");
}else {
//var gates = bmonmate.gates_status(connector, "TND1", ".*");
//console.log('\n' + gates[0])
//console.log('\n' + gates[1])
var config = bmonmate.gateway_config(connector, "TND1", "Binance.PROD");
console.log('\n' + config.length);
}
}


