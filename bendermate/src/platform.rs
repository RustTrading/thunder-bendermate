use std::{
    ffi::CStr,
    os::raw::{c_char, c_ushort, c_ulong, c_double, c_int, c_void},
    ptr,
    convert::TryInto,
    ffi::CString,
    str,
    mem,
    io::Read,
    slice::{from_raw_parts, from_raw_parts_mut},
};

use std::net::{TcpStream};
mod network;

#[derive(Debug, PartialEq, Clone, Copy)] 
pub enum ConnectionType {
    Tcp = 0,
    Udp = 1,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct DaemonConnector {
    api_ptr: *mut c_void,
    data_buffer_size: c_ulong,
    version : c_int,
    buffer : *mut u8,
    connection_type : c_int,
}

#[repr(C)]
pub struct RBook {
    bbook: *mut c_void, 
}

#[repr(C)]
pub struct RDeal {
    bdeal: *mut c_void, 
}

#[repr(C)]
pub struct RFormula {
    bformula: *mut c_void, 
}

#[repr(C)]
pub struct RVariable {
    bvariable: *mut c_void, 
}

#[repr(C)]
pub struct DataConnector {
    api_ptr: *mut c_void,
    on_book: extern fn(rb: *mut RBook),
    on_variable: extern fn(rv : *mut RVariable),
    on_deal: extern fn(rd : *mut RDeal),
    on_formula: extern fn(rd : *mut RFormula),
}

#[derive(Debug)]
#[repr(C)]
pub struct RobotStatus {
    name: *mut c_char,
    config: *mut c_char,
    next_rstatus: *mut RobotStatus,
    prev_rstatus: *mut RobotStatus,
}

impl RobotStatus {
    pub fn new() -> RobotStatus {
        RobotStatus {
            name: ptr::null_mut(),
            config: ptr::null_mut(),
            next_rstatus: ptr::null_mut(),
            prev_rstatus: ptr::null_mut(),
        }
    }

    pub fn dump(&self) {
        unsafe {
            let mut next: *const RobotStatus = self;
            while next != ptr::null() {
                let c_name: &CStr = CStr::from_ptr((*next).name);
                let c_config: &CStr = CStr::from_ptr((*next).config);
                println!(
                    "name: {}, config: {}",
                    c_name.to_str().unwrap(),
                    c_config.to_str().unwrap()
                );
                next = (*next).next_rstatus;
            }
        }
    }
}

impl Drop for RobotStatus {
    fn drop(&mut self) {
        unsafe {
            let next: *mut RobotStatus = self.next_rstatus;
            Box::from_raw(self.name);
            Box::from_raw(self.config);
            if next != ptr::null_mut() {
                Box::from_raw(next);
            }
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct SensorEvent {
    name: *mut c_char,
    category: *mut c_char,
    dvalue: c_double,
    svalue: *mut c_char,
    moment: ::std::os::raw::c_longlong,
    next_sevent: *mut SensorEvent,
    prev_sevent: *mut SensorEvent,
}

impl SensorEvent {
    pub fn new() -> SensorEvent {
        SensorEvent {
            name: ptr::null_mut(),
            category: ptr::null_mut(),
            dvalue: -1.0,
            svalue: ptr::null_mut(),
            moment: 0,
            next_sevent: ptr::null_mut(),
            prev_sevent: ptr::null_mut(),
        }
    }

    pub fn dump(&self) {
        unsafe {
            let mut next: *const SensorEvent = self;
            while next != ptr::null() {
                let c_name: &CStr = CStr::from_ptr((*next).name);
                let c_category: &CStr = CStr::from_ptr((*next).category);
                let sval: &CStr = CStr::from_ptr((*next).svalue);
                let dval = (*next).dvalue;
                let tval = (*next).moment;
                println!(
                    "name: {}, category: {}, svalue: {} dvalue: {} timetick: {}",
                    c_name.to_str().unwrap(),
                    c_category.to_str().unwrap(),
                    sval.to_str().unwrap(),
                    dval,
                    tval
                );
                next = (*next).next_sevent;
            }
        }
    }
}

impl Drop for SensorEvent {
    fn drop(&mut self) {
        unsafe {
            let next: *mut SensorEvent = self.next_sevent;
            Box::from_raw(self.name);
            Box::from_raw(self.category);
            Box::from_raw(self.svalue);
            if next != ptr::null_mut() {
                Box::from_raw(next);
            }
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct RobotOrder {
    robot: *mut c_char,
    account_name: *mut c_char,
    instrument_name: *mut c_char,
    side: *mut c_char,
    size_base: c_double,
    size_quote: c_double,
    next_rorder: *mut RobotOrder,
    prev_rorder: *mut RobotOrder,
}

impl RobotOrder {
    pub fn new() -> RobotOrder {
        RobotOrder {
            robot: ptr::null_mut(),
            account_name: ptr::null_mut(),
            instrument_name: ptr::null_mut(),
            side: ptr::null_mut(),
            size_base: -1.0,
            size_quote: -1.0,
            next_rorder: ptr::null_mut(),
            prev_rorder: ptr::null_mut(),
        }
    }

    pub fn dump(&self) {
        unsafe {
            let mut next: *const RobotOrder = self;
            while next != ptr::null() {
                let c_robot: &CStr = CStr::from_ptr((*next).robot);
                let c_account_name: &CStr = CStr::from_ptr((*next).account_name);
                let c_instrument_name: &CStr = CStr::from_ptr((*next).instrument_name);
                let c_side: &CStr = CStr::from_ptr((*next).side);
                let size_base = (*next).size_base;
                let size_quote = (*next).size_quote;
                println!(
                    "robot: {}, account_name: {}, instrument_name: {}, side: {}, size_base: {}, size_quote: {}",
                    c_robot.to_str().unwrap(),
                    c_account_name.to_str().unwrap(),
                    c_instrument_name.to_str().unwrap(),
                    c_side.to_str().unwrap(),
                    size_base,
                    size_quote,
                );
                next = (*next).next_rorder;
            }
        }
    }
}

impl Drop for RobotOrder {
    fn drop(&mut self) {
        unsafe {
            let next: *mut RobotOrder = self.next_rorder;
            Box::from_raw(self.robot);
            Box::from_raw(self.account_name);
            Box::from_raw(self.instrument_name);
            Box::from_raw(self.side);
            if next != ptr::null_mut() {
                Box::from_raw(next);
            }
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct SensorVariable {
    name: *mut c_char,
    value: c_double,
    next_svar: *mut SensorVariable,
    prev_svar: *mut SensorVariable,
}

impl SensorVariable {
    pub fn new() -> SensorVariable {
        SensorVariable {
            name: ptr::null_mut(),
            value: -1.0,
            next_svar: ptr::null_mut(),
            prev_svar: ptr::null_mut(),      
        }
    }

    pub fn dump(&self) {
        unsafe {
            let mut next: *const SensorVariable = self;
            while next != ptr::null() {
                let c_name: &CStr = CStr::from_ptr((*next).name);
                let c_value = (*next).value;
                println!(
                    "name: {}, value: {}",
                    c_name.to_str().unwrap(),
                    c_value,     
                );
                next = (*next).next_svar;
            }
        }
    }
}

impl Drop for SensorVariable {
    fn drop(&mut self) {
        unsafe {
            let next: *mut SensorVariable = self.next_svar;
            Box::from_raw(self.name);
            if next != ptr::null_mut() {
                Box::from_raw(next);
            }
        }
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct SensorPosition {
    account: *mut c_char,
    name: *mut c_char,
    iclass: *mut c_char,
    gateway: *mut c_char,
    value: c_double,
    next_spos: *mut SensorPosition,
    prev_spos: *mut SensorPosition,
}

impl SensorPosition {
    pub fn new() -> SensorPosition {
        SensorPosition {
            account: ptr::null_mut(),
            name: ptr::null_mut(),
            iclass: ptr::null_mut(),
            gateway: ptr::null_mut(),
            value: -1.0,
            next_spos: ptr::null_mut(),
            prev_spos: ptr::null_mut(),      
        }
    }

    pub fn dump(&self) {
        unsafe {
            let mut next: *const SensorPosition = self;
            while next != ptr::null() {
                let c_account: &CStr = CStr::from_ptr((*next).account);
                let c_name: &CStr = CStr::from_ptr((*next).name);
                let c_iclass: &CStr = CStr::from_ptr((*next).iclass);
                let c_gateway: &CStr = CStr::from_ptr((*next).gateway);
                let c_value = (*next).value;
                println!(
                    "account: {}, name: {}, iclass: {}, gateway: {}, value: {}",
                    c_account.to_str().unwrap(),
                    c_name.to_str().unwrap(),
                    c_iclass.to_str().unwrap(),
                    c_gateway.to_str().unwrap(),
                    c_value,     
                );
                next = (*next).next_spos;
            }
        }
    }
}

impl Drop for SensorPosition {
    fn drop(&mut self) {
        unsafe {
            let next: *mut SensorPosition = self.next_spos;
            Box::from_raw(self.name);
            if next != ptr::null_mut() {
                Box::from_raw(next);
            }
        }
    }
}

pub fn create_daemon_connector() -> *mut DaemonConnector {
         let mut dc  = unsafe { Box::new(*create_daemon_connector_impl()) };
         dc.data_buffer_size = 65535;
         dc.version = 0;
         return Box::into_raw(dc);
}

extern "C" {
    pub fn run_file_script(argc : c_int, argv : *const *const c_char);
}

#[no_mangle]
pub extern "C" fn run_js_script(filename : *const c_char) {
    let c_str: &CStr = unsafe { CStr::from_ptr(filename) };
    let str_slice: &str = c_str.to_str().unwrap();
    let f = CString::new(str_slice).unwrap();
    let qjs = CString::new("qjs").unwrap();
    let argv = [qjs.as_ptr(), f.as_ptr()];
    unsafe {
        run_file_script(2, argv.as_ptr());
    }
}

#[no_mangle]
pub extern "C" fn login_c(dc: *mut DaemonConnector, user : *const c_char, password: *const c_char) {
    let c_user: &CStr = unsafe { CStr::from_ptr(user) };
    let user_slice: &str = c_user.to_str().unwrap();
    let c_password: &CStr = unsafe { CStr::from_ptr(password) };
    let password_slice: &str = c_password.to_str().unwrap();
    unsafe {
        let tcpstream  = (*dc).api_ptr as *mut TcpStream;
        let _bmess = network::login(user_slice, password_slice, &mut *tcpstream);
        //let reply = format!("{:#?}", bmess);
        //ptr::copy_nonoverlapping(reply.as_ptr(), (*dc).buffer, reply.len());
        let mut class_stream = String::new();
        network::listen_bender(&mut class_stream,  &mut *tcpstream, &network::deserialize_class); 
        println!("bender stream: {:?}", class_stream);
        println!("end of stream");
        network::pongMessage(&mut *tcpstream);
    }    
}

#[no_mangle]
pub extern "C" fn get_sensors_c(dc: *mut DaemonConnector, server : *const c_char) {
    let server_c: &CStr = unsafe { CStr::from_ptr(server) };
    let server_slice: &str = server_c.to_str().unwrap();
    let mut class_stream = String::new();
    unsafe {    
        let tcpstream : *mut TcpStream = (*dc).api_ptr as *mut TcpStream;
        network::bmethod1("sensors", network::encodeString(server_slice),  &mut *tcpstream);  
        let mut buff_slice = unsafe { from_raw_parts_mut((*dc).buffer, 65535) };
        let result = (*tcpstream).read(&mut buff_slice).unwrap();
        println!("{:?}", &buff_slice[0..result]);
        println!("{:?} {:?}", network::deserialize_header(&buff_slice), result);
        let decoded = network::decode_sensors(&buff_slice[14..result]);
        let mut sensor_offset = Box::new(0);
        let _version = network::get_uint32_from_buf(&decoded[*sensor_offset..]);
        *sensor_offset += 4;
        let sensorsize = network::get_uint32_from_buf(&decoded[*sensor_offset..]);
        *sensor_offset += 4;           
        for _ in 0..sensorsize {
            let sensor = network::parse_sensor(&decoded, &mut sensor_offset);
            class_stream.push_str(&format!("{:?}", &sensor)[..]);
        } 
        println!("{:?}", &class_stream); 
        network::pongMessage(&mut *tcpstream);
    }
       // ptr::copy_nonoverlapping(replystr.as_ptr(), (*dc).buffer, replystr.len() + 1);
}

#[no_mangle]
pub extern "C" fn set_bind_c(dc: *mut DaemonConnector, server : *const c_char, variable : *const c_char) {
    let server_c: &CStr = unsafe { CStr::from_ptr(server) };
    let server_slice: &str = server_c.to_str().unwrap();
    let variable_c: &CStr = unsafe { CStr::from_ptr(variable) };
    let variable_slice: &str = variable_c.to_str().unwrap();
    let mut class_stream = String::new();
    unsafe {    
        let tcpstream : *mut TcpStream = (*dc).api_ptr as *mut TcpStream;
        network::bmethod22("bind", network::encodeString(server_slice), network::encodeBase64(variable_slice),  &mut *tcpstream);   
       // (*tcpstream).read(&mut buff_slice);
        network::listen_bender(&mut class_stream,  &mut *tcpstream, &network::deserialize_class); 
        println!("bender stream: {:?}", class_stream);
        println!("end of stream");
        network::pongMessage(&mut *tcpstream);
    }
       // ptr::copy_nonoverlapping(replystr.as_ptr(), (*dc).buffer, replystr.len() + 1);
}

#[no_mangle]
pub extern "C" fn set_variable_c(dc: *mut DaemonConnector, timestamp : u64, variable : *const c_char, value : c_double) {
    let variable_c: &CStr = unsafe { CStr::from_ptr(variable) };
    let variable_slice: &str = variable_c.to_str().unwrap();
    let mut class_stream = String::new();
    unsafe {    
        let tcpstream : *mut TcpStream = (*dc).api_ptr as *mut TcpStream;
        network::bmethod33("setVariableValue", timestamp, network::encodeBase64(variable_slice), value as f64, &mut *tcpstream);   
       // (*tcpstream).read(&mut buff_slice);
        network::listen_bender(&mut class_stream,  &mut *tcpstream, &network::deserialize_class); 
        println!("bender stream: {:?}", class_stream);
        println!("end of stream");
        network::pongMessage(&mut *tcpstream);
    }    
}

#[no_mangle]
pub extern "C" fn get_daemon_connector_c(server : *const c_char, port: *const c_char, contype : ConnectionType) -> *mut DaemonConnector {
    let buff = vec![0u8; 65535];
    let buff_boxed = buff.into_boxed_slice();
    let mut dc = Box::new(DaemonConnector{ api_ptr:  ptr::null_mut(), data_buffer_size: 65535, 
        buffer :  Box::into_raw(buff_boxed) as *mut u8, version : 1, connection_type : contype as i32});
    let server_c: &CStr = unsafe { CStr::from_ptr(server) };
    let server_slice: &str = server_c.to_str().unwrap();
    let port_c: &CStr = unsafe { CStr::from_ptr(port) };
    let port_slice: &str = port_c.to_str().unwrap();
    let buff_slice = unsafe { from_raw_parts_mut(dc.buffer, 65535) };
    if contype == ConnectionType::Tcp {
        let tcstream_box = Box::new(network::connect(server_slice, port_slice, buff_slice));
        dc.api_ptr = Box::into_raw(tcstream_box) as *mut c_void;
    }
    if contype == ConnectionType::Udp {

    }
    Box::into_raw(dc)
}

#[no_mangle]
pub extern "C" fn get_gateway_config_c(
    dc: *mut DaemonConnector, 
    server: *const c_char, 
    gateway: *const c_char) {     
        let server_c: &CStr = unsafe { CStr::from_ptr(server) };
        let server_slice: &str = server_c.to_str().unwrap();
        let gateway_c: &CStr = unsafe { CStr::from_ptr(gateway) };
        let gateway_slice: &str = gateway_c.to_str().unwrap();
    
    unsafe {   
    
    if (*dc).version == 0 {
        gateway_config_impl(dc, server, gateway, (*dc).buffer);
    }    
        
    if (*dc).version == 1 {    
        let tcpstream : *mut TcpStream = (*dc).api_ptr as *mut TcpStream;
        network::bmethod2("gatewayConfig", network::encodeString(server_slice), gateway_slice, &mut *tcpstream);   
       // (*tcpstream).read(&mut buff_slice);
        let mut class_stream = String::new();
        network::listen_bender(&mut class_stream,  &mut *tcpstream, &network::deserialize_class); 
        println!("bender stream: {:?}", class_stream);
        println!("end of stream");
        network::pongMessage(&mut *tcpstream);
        }
       // ptr::copy_nonoverlapping(replystr.as_ptr(), (*dc).buffer, replystr.len() + 1);
    }
    }

#[no_mangle]
pub extern "C" fn get_all_orders_c(dc: *mut DaemonConnector, server: *const  c_char) {
    let server_c: &CStr = unsafe { CStr::from_ptr(server) };
    let server_slice: &str = server_c.to_str().unwrap();
    unsafe {   
        let buff_slice = { from_raw_parts_mut((*dc).buffer, 65535) }; 
        let tcpstream : *mut TcpStream = (*dc).api_ptr as *mut TcpStream;
        network::bmethod1("getAllOrders", network::encodeString(server_slice), &mut *tcpstream);   
       // (*tcpstream).read(&mut buff_slice);
        let mut class_stream = String::new();
        network::listen_bender(&mut class_stream,  &mut *tcpstream, &network::deserialize_class); 
        println!("bender stream: {:?}", class_stream);
        println!("end of stream");
        network::pongMessage(&mut *tcpstream);
    }
}

extern "C" {

    pub fn create_daemon_connector_impl() -> *mut DaemonConnector ;

    pub fn delete_daemon_connector(dc: *mut DaemonConnector);
   
    pub fn create_data_connector(
        port : c_int, 
        on_book: extern fn(rb: *mut RBook),
        on_deal: extern fn(rd : *mut RDeal),
        on_variable: extern fn(rv : *mut RVariable),
        on_formula: extern fn(rd : *mut RFormula)
    ) -> *mut DataConnector;

    pub fn gateway_config_impl(
        dc: *mut DaemonConnector, 
        server: *const c_char, 
        gateway: *const c_char,
        stringBuffer: *mut u8,
    );
    
    pub fn connect_to_daemon(
        dc: *mut DaemonConnector,
        host_w: *const c_char,
        port: c_ushort,
    ) -> c_int;

    pub fn login(
        dc: *mut DaemonConnector, 
        user: *const c_char, 
        password: *const c_char
    ) -> c_int;

    pub fn login_by_hash(
        dc: *mut DaemonConnector, 
        user: *const c_char, 
        hash: *const c_char
    ) -> c_int;

    pub fn gates_status(
        dc: *mut DaemonConnector,
        server: *const c_char,
        gways: *const c_char,
        slist: *mut RobotStatus,
    );

    pub fn robots_status(
        dc: *mut DaemonConnector,
        server: *const c_char,
        robots: *const c_char,
        slist: *mut RobotStatus,
    );

    pub fn add_robot(
        dc: *mut DaemonConnector, 
        server: *const c_char, 
        robot: *const c_char, 
        config: *const c_char
    );
    
    pub fn start_robot(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        robot: *const c_char
    );   
    
    pub fn start_robots(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        robots: *const c_char
    ); 
    
    pub fn stop_robot(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        robot: *const c_char
    );   
    
    pub fn stop_robots(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        robots: *const c_char
    ); 
   
    pub fn delete_robot(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        robot: *const c_char
    ); 

    pub fn start_gateway(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        gateway: *const c_char
    );   
    
    pub fn start_gateways(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        gateways: *const c_char
    ); 
    
    pub fn stop_gateway(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        gateway: *const c_char
    );   
    
    pub fn stop_gateways(
        dc: *mut DaemonConnector,
        server: *const c_char, 
        gateways: *const c_char
    ); 

    pub fn get_all_orders(
        dc: *mut DaemonConnector,
        server: *const c_char,
        robot: *const c_char,
        rlist: *mut RobotOrder,
    );

    pub fn sensors(
        rc: *mut DaemonConnector, 
        server: *const c_char, 
        sevent: *mut SensorEvent,
    );

    pub fn variables(
        rc: *mut DaemonConnector, 
        server: *const c_char, 
        svar: *mut SensorVariable,
    );

    pub fn positions(
        rc: *mut DaemonConnector,
        server: *const c_char,
        spos: *mut SensorPosition,
    );
}
