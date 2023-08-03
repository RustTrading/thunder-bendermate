use std::ffi::CString;
extern crate clap;
use clap::{Arg, App};
use std::os::raw::{c_char, c_int, c_ushort, c_ulong, c_double, c_void};
use std::ptr;

const FORMULA_LIST_CONFIG: &str = "
<FormulaListConfig>
    <Variables>
        <Variable Name=\"binSafe1\" Value=\"[binSafeMod]\" Sensor=\"true\" Public=\"true\" />
    </Variables>
</FormulaListConfig>";

#[repr(C)]
#[derive(Clone, Copy)]
pub struct DaemonConnector {
    api_ptr: *mut c_void,
    data_buffer_size: c_ulong,
    version : c_int,
    buffer : *mut u8,
}

extern "C" {
    pub fn run_js_script(filename : *const c_char);
    pub fn login_c(daemonconnector : *mut DaemonConnector, username : *const c_char, password : *const c_char);
    pub fn get_gateway_config_c(daemonconnector : *mut DaemonConnector, server : *const c_char, gateway : *const c_char);
    pub fn get_daemon_connector_c(server : *const c_char, port: *const c_char)->*mut DaemonConnector; 
}

fn main()  -> std::io::Result<()> {
    // Temporary credentials.
    // let (user, password) = ("pkorotkov", "vVAUMhjatkw97CvBknsR");
    let matches = App::new("bendermate_js_api")
        .version("0.1.0")
        .author("Thunder & Co")
        .about("runs js scripts to interact with bender")
        .arg(Arg::with_name("jsfile")
        .short("f")
        .long("file")
        .required(false)
        .takes_value(true)
        .help("js scriptfile")).get_matches();
    
    let filename = matches.value_of("jsfile");
    if let Some(f) = filename {
        let f = CString::new(f).unwrap();
        unsafe {
            run_js_script(f.as_ptr());
        }
        return Ok(());
    }

    let (server, port) = ("dmx1.loc", "8050");
    
    let (user, password) = ("msamsonov", "7SqiiFUNWM5NrMirSHHh");

    let server_c = CString::new(server).unwrap();
    let port_c = CString::new(port).unwrap();
    let port_data_c = CString::new("3099").unwrap();

    let user_c = CString::new(user).unwrap();
    let password_c = CString::new(password).unwrap();

    let gateway = CString::new("Binance.PROD").unwrap();
    let server_bender = CString::new("TND1").unwrap();

    unsafe {
        let dc : *mut DaemonConnector = get_daemon_connector_c(server_c.as_ptr(), port_c.as_ptr());
        login_c(dc, user_c.as_ptr(), password_c.as_ptr()); 
        let ptr1 = server_bender.as_ptr() as *const i8;
        let ptr2 = gateway.as_ptr() as *const i8;
        get_gateway_config_c(dc, server_bender.as_ptr(), gateway.as_ptr());
        
        let datac : *mut DaemonConnector = get_daemon_connector_c(server_c.as_ptr(), port_data_c.as_ptr());
        login_c(datac, user_c.as_ptr(), password_c.as_ptr());
    }
   /* let bender_daemon_address = ("dmx1.loc", 8050);
    let bender_server_name = "TND1"; 
    
    let dc: *mut DaemonConnector = unsafe {
        platform::create_daemon_connector()
    };
    unsafe {
        let host = CString::new(bender_daemon_address.0).unwrap();
        let port = bender_daemon_address.1;
        let success = platform::connect_to_daemon(dc, host.as_ptr(), port);
        if success == -1 {
            eprintln!("failed to find bender daemon");
            std::process::exit(1);
        }
    }
    unsafe {
        let u = CString::new(user).unwrap();
        let p = CString::new(password).unwrap();
        let authorized = platform::login(dc, u.as_ptr(), p.as_ptr());
        if authorized != 0 {
            eprintln!("failed to access bender daemon");
            std::process::exit(2);
        }
    };
    let server_name = CString::new(bender_server_name).unwrap();
    let gates = CString::new(".*").unwrap();
    let robots = CString::new(".*").unwrap();

    let glist = Box::into_raw(Box::new(RobotStatus::new()));
    let rlist = Box::into_raw(Box::new(RobotStatus::new()));
    let slist = Box::into_raw(Box::new(SensorEvent::new()));
    unsafe {
       // platform::gates_status(dc, server_name.as_ptr(), gates.as_ptr(), glist);
        // (*glist).dump();
       // platform::robots_status(dc, server_name.as_ptr(), robots.as_ptr(), rlist);
        // (*rlist).dump();
    };
    let config = CString::new(FORMULA_LIST_CONFIG.trim_start()).unwrap();
    let robot_name = CString::new("FormulaList.test-01").unwrap();
    unsafe {
      //  platform::add_robot(dc, server_name.as_ptr(), robot_name.as_ptr(), config.as_ptr());
        // sensors(dc, server_name.as_ptr(), slist);
        // (*slist).dump();
        platform::delete_daemon_connector(dc);
        Box::from_raw(glist);
        Box::from_raw(rlist);
        Box::from_raw(slist);
    };  */
 
    Ok(())
}
