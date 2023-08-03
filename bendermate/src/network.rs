//use async_std::net::TcpStream;
use std::net::{TcpStream, UdpSocket};
use std::io::{Read, Write};
use serde::{Serialize, Deserialize};
use std::fmt;
extern crate bincode;
extern crate openssl_sys;
use std::os::raw::{c_int, c_uchar};
use std::ffi::CString;
use xmltree::{Element, XMLNode};
use libflate::zlib::{Decoder};
use std::fmt::Debug;
use std::str;
use base64::{encode, decode};

#[derive(Debug)] 
enum FLAG_ENUM
{
    FLAG_METHOD = 1,
    FLAG_ERROR  = 2,
    FLAG_KEEP_ALIVE_PING  = 4,
    FLAG_KEEP_ALIVE_PONG  = 8,
    FLAG_SNAPPY_USED = 16,   // Message being send is snappied.
    FLAG_SNAPPY_SUPPORTED = 32,    // Set to 1 to indicate that you can understand snappy.
}

#[derive(Debug)] 
enum MARKER_ENUM {
    MARKER_ONE_WAY = 0xFFFFFFFF,
}

#[derive(Debug)] 
enum PacketType
{
    PT_CALL,
    PT_REPLY,
    PT_ERROR,
    PT_PING,
    PT_PONG
}

#[derive(Debug)] 
enum ServiceProtocol
{
    kProtoBinary,
    kProtoJsonRpc
}

#[derive(Debug, PartialEq)] 
pub enum BObjectType
{
    String,
    u32,
    u64,
    f64,
}

pub trait BObject : Debug {
    fn data(&self) -> Vec::<u8>;
    fn btype(&self) -> BObjectType;
}

impl BObject for String {
    fn data(&self) -> Vec::<u8> { 
        let mut vec = Vec::new();
        vec.extend_from_slice(self.as_bytes());
        vec
    }
    fn btype(&self) -> BObjectType { BObjectType::String }
}

impl BObject for u32 {
    fn data(&self)->Vec::<u8> { 
        let mut vec = Vec::new();
        let bytes = self.to_le_bytes();
        vec.extend_from_slice(&bytes);
        vec
    }
    fn btype(&self) -> BObjectType { BObjectType::u32 }
}

impl BObject for u64 {
    fn data(&self)->Vec::<u8> { 
        let mut vec = Vec::new();
        let bytes = self.to_le_bytes();
        vec.extend_from_slice(&bytes);
        vec
    }
    fn btype(&self) -> BObjectType { BObjectType::u64 }
}

impl BObject for f64 {
    fn data(&self)->Vec::<u8> { 
        let mut vec = Vec::new();
        let bytes = self.to_le_bytes();
        vec.extend_from_slice(&bytes);
        vec
    }
    fn btype(&self) -> BObjectType { BObjectType::f64 }
}

pub struct PayLoad {
    method_id : String,
    params : Vec<Box<dyn BObject>>,   
}

impl std::fmt::Debug for PayLoad {
    fn fmt(&self, fmt: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        write!(fmt, "method_id : {}, params : {:#?}", self.method_id, self.params)
    }
}

#[derive(Debug)] 
pub struct BMessage {
    pub mheader : MessageHeader,
    pub mpayload : Option<PayLoad>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct MessageHeader {
	pub flags : u16,
    pub bodySize : u32,
    pub marker : u32,
}

impl fmt::LowerHex for MessageHeader {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let val = self.flags;
        write!(f, "flags: 0x{:x}, bodySize: 0x{:x}, marker: 0x{:x}", self.flags, self.bodySize, self.marker)
    }
}

pub fn serialize_to_buf(data : &[u8], buf : &mut [u8], data_type : BObjectType) -> usize {   
    if data_type == BObjectType::String {
        let dsize  = data.len() as u32;
        let ubytes = dsize.to_le_bytes();
        buf[0..4].clone_from_slice(&ubytes);
        buf[4..4 + dsize as usize].clone_from_slice(data);   
        return 4 + dsize as usize; 
    }

    if  data_type == BObjectType::u32 || data_type == BObjectType::u64 {
        buf[0..data.len()].clone_from_slice(data);   
        return data.len();   
    }

    0
}

pub fn serialize_header_to_buf(mheader : &MessageHeader, buf : &mut[u8]) -> usize {
    let flags: u16 = mheader.flags;
    let fbytes = flags.to_le_bytes();
    buf[0..2].clone_from_slice(&fbytes);
    let bodySize: u32 = mheader.bodySize;
    let bsbytes = bodySize.to_le_bytes();
    buf[2..6].clone_from_slice(&bsbytes);
    let marker : u32 = mheader.marker;
    let mbytes = marker.to_le_bytes();
    buf[6..10].clone_from_slice(&mbytes);
    10
}

pub fn get_string_from_buf(buf : &[u8]) -> String {
    let mut arr = [0u8; 4];
    arr.clone_from_slice(&buf[0..4]);
    let slength : usize = u32::from_le_bytes(arr) as usize;
   if (slength == 0) {
       return String::new();
    }
    let res =  String::from_utf8(buf[4..4 + slength].to_vec());
    res.unwrap()
}

pub fn get_uint64_from_buf(buf : &[u8]) -> u64 {
    let mut arr = [0u8; 8];
    arr.clone_from_slice(&buf[0..8]);
    u64::from_le_bytes(arr)
}

pub fn get_uint32_from_buf(buf : &[u8]) -> u32 {
    let mut arr = [0u8; 4];
    arr.clone_from_slice(&buf[0..4]);
    u32::from_le_bytes(arr)
}

pub fn get_uint16_from_buf(buf : &[u8]) -> u16 {
    let mut arr = [0u8; 2];
    arr.clone_from_slice(&buf[0..2]);
    u16::from_le_bytes(arr)
}

pub fn get_bool_from_buf(buf : &[u8]) -> bool {
    buf[0] != 0
}

pub fn get_f64_from_buf(buf : &[u8]) -> f64 {
    let mut arr = [0u8; 8];
    arr.clone_from_slice(&buf[0..8]);
    f64::from_le_bytes(arr)
}

pub fn get_stream_from_buf(buf: &[u8]) -> String {
    let mut str_buf = String::new();
    for i in 0..buf.len() {
        str_buf.push(buf[i] as char);
    }
    str_buf
}

pub fn get_xml_from_buf(buf : &[u8], offset : &mut usize) -> Vec<XMLNode> {
    let xml_string = get_string_from_buf(buf);
    *offset+= 4 + xml_string.len();
    Element::parse_all(xml_string.as_bytes()).unwrap()
}

pub fn deserialize_from_buf(buf : &[u8]) -> PayLoad {
    let param1_str = get_string_from_buf(&buf); 
    println!("{:?}", param1_str);
    let param2_str = get_string_from_buf(&buf[4 + param1_str.len()..]);
    println!("{:?}", param2_str);
    let param3_str = get_string_from_buf(&buf[2 * 4 + param2_str.len() + param1_str.len()..]);    
    println!("{:?}", param3_str);
    let mut param_vec: Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2_str));
    param_vec.push(Box::new(param3_str));
    PayLoad{method_id : param1_str, params : param_vec}
}

pub fn deserialize_from_buf2(buf : &[u8]) -> PayLoad {
    let param1_str = get_string_from_buf(&buf); 
    let param2_str = get_string_from_buf(&buf[4 + param1_str.len()..]);      
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2_str));
    PayLoad{method_id : param1_str, params : param_vec}
}

pub fn deserialize_from_buf1(buf : &[u8]) -> PayLoad {
    let param1_str = get_string_from_buf(&buf); 
    PayLoad{method_id : param1_str, params : Vec::new()}
}

pub fn deserialize_header(buf : &[u8]) -> MessageHeader {
    let mut arr = [0u8; 2];
    arr.clone_from_slice(&buf[0..2]);
    let fl = u16::from_le_bytes(arr);
    let mut arr2 = [0u8; 4];
    arr2.clone_from_slice(&buf[2..6]);
    let bs = u32::from_le_bytes(arr2);
    let mut arr3 = [0u8; 4];
    arr3.clone_from_slice(&buf[6..10]);
    let mk = u32::from_le_bytes(arr3);
    MessageHeader{ flags :  fl, bodySize: bs, marker: mk}
}

pub fn serialize_payload_to_buf(mpayload : &PayLoad, mut buf : &mut [u8]) -> usize {
    let mut size :usize = 0;
    size += serialize_to_buf(mpayload.method_id.as_bytes(), &mut buf, BObjectType::String);

    for val in &mpayload.params {
        size += serialize_to_buf(&*val.data(), &mut buf[size..], val.btype());
    }
    size   
}

pub fn serialize_message(mut bmess: &mut BMessage, mut buf : &mut [u8]) -> usize {
    let pyld = bmess.mpayload.as_ref().unwrap();
    let size = serialize_payload_to_buf(&pyld, &mut buf[10..]);
    bmess.mheader.bodySize = size as u32;
    serialize_header_to_buf(&bmess.mheader, &mut buf);
    size + 10
}

#[derive(PartialEq, Debug)] 
pub struct DaemonConfig {
    config : Option<Vec<xmltree::XMLNode>>,
}

#[derive(PartialEq, Debug)] 
pub struct BRobotStatus {
    version : Option<u32>,
    status : Option<u16>,
    flags : Option<u16>,
    name : Option<String>,
    config : Option<Vec<xmltree::XMLNode>>,
    message: Option<String>,
    strategy: Option<String>,
}

#[derive(PartialEq, Debug)] 
pub struct BIndicator {
    timestamp : Option<u64>,
    server : Option<String>,
    name : Option<String>,
    value : Option<f64>,
}

fn read_indicator(buf : &[u8], offset : &mut usize) -> BIndicator {
    
    let mut bindicator = BIndicator {
        timestamp : None, 
        server : None, 
        name : None, 
        value : None, 
    };
 
    bindicator.timestamp = Some(get_uint64_from_buf(&buf[*offset..]));
    *offset += 8;
    bindicator.server = Some(get_stream_from_buf(&buf[*offset..*offset + 4]));
    *offset += 4;
    bindicator.name = Some(decodeBase64(&get_stream_from_buf(&buf[*offset..*offset + 8])));
    *offset += 8;
    bindicator.value = Some(get_f64_from_buf(&buf[*offset..]));
    *offset += 8;
    bindicator
 }

fn read_robot_status(buf : &[u8], offset : &mut usize) -> BRobotStatus {
   let mut rstatus = BRobotStatus{
        version : None, 
        status: None, 
        flags: None, 
        name: None, 
        config : None, 
        message: None, 
        strategy: None};

   rstatus.version = Some(get_uint32_from_buf(&buf[*offset..]));
   *offset += 4;
   rstatus.status = Some(get_uint16_from_buf(&buf[*offset..]));
   *offset += 2;
   rstatus.flags = Some(get_uint16_from_buf(&buf[*offset..]));
   *offset += 2;
   rstatus.name = Some(get_string_from_buf(&buf[*offset..]));
   *offset += 4 + rstatus.name.as_ref().unwrap().len();
   rstatus.config = Some(get_xml_from_buf(&buf[*offset..], offset));
   rstatus.message = Some(get_string_from_buf(&buf[*offset..]));
   *offset += 4 + rstatus.message.as_ref().unwrap().len();
   rstatus.strategy = Some(get_string_from_buf(&buf[*offset..]));
   *offset += 4 + rstatus.strategy.as_ref().unwrap().len();
   rstatus
}

#[derive(PartialEq, Debug)] 
pub struct BServerStatus {
    version : Option<u32>,
    name : Option<String>,
    management_online : Option<bool>,
    remote_core_online : Option<bool>,
    runtime_config : Option<Vec<xmltree::XMLNode>>,
    revision : Option<String>,
    was_shust_down : Option<bool>,
}

fn read_server_status(buf : &[u8], offset : &mut usize) -> BServerStatus {
   let mut sstatus = BServerStatus{
    version : None,
    name : None,
    management_online : None,
    remote_core_online : None,
    runtime_config : None,
    revision : None,
    was_shust_down : None,
   };
   sstatus.version = Some(get_uint32_from_buf(&buf[*offset..]));
   *offset += 4;
   //println!("{:?}",sstatus);
   sstatus.name = Some(get_stream_from_buf(&buf[*offset..*offset + 4]));
   *offset += 4;
   //println!("{:?}",sstatus);
   sstatus.management_online = Some(get_bool_from_buf(&buf[*offset..]));
   *offset +=1;
   //println!("{:?}",sstatus);
   sstatus.remote_core_online = Some(get_bool_from_buf(&buf[*offset..]));
   *offset += 1;
   //println!("{:?}",sstatus);
   sstatus.runtime_config = Some(get_xml_from_buf(&buf[*offset..], offset));
   //println!("{:?}",sstatus);
   sstatus.revision = Some(get_string_from_buf(&buf[*offset..]));
   *offset += 4 + sstatus.revision.as_ref().unwrap().len();
   //println!("{:?}",sstatus);
   sstatus.was_shust_down = Some(get_bool_from_buf(&buf[*offset..]));
   *offset += 1;
   //println!("{:?}",sstatus);
   sstatus
}

fn read_daemon_status(buf : &[u8], offset : &mut usize) -> DaemonConfig {
    let mut bdstatus = DaemonConfig{
     config : None,
    };
    bdstatus.config = Some(get_xml_from_buf(&buf[*offset..], offset));
    //println!("{:?}",bdstatus);
    bdstatus
 }

pub fn parse_sensors(buf : &[u8], class_stream : &mut String, offset : &mut usize) {
    let _servername = get_stream_from_buf(&buf[*offset..*offset + 4]);
    *offset+=4;
    let _chuncksize = get_uint32_from_buf(&buf[*offset..]);
    *offset+=4;
    let decoded = decode_sensors(&buf[*offset ..]);
    let mut sensor_offset = Box::<usize>::new(0);
    let _version = get_uint32_from_buf(&decoded[*sensor_offset..]);
    *sensor_offset += 4;
    let sensorsize = get_uint32_from_buf(&decoded[*sensor_offset..]);
    *sensor_offset += 4;           
    for _ in 0..sensorsize {
        let sensor = parse_sensor(&decoded, &mut sensor_offset);
        class_stream.push_str(&format!("{:?}", &sensor)[..]);
    }
}

pub fn parse_sensors2(buf : &[u8], class_stream : &mut String, offset : &mut usize) {
    let decoded = decode_sensors(&buf[*offset ..]);
    let mut sensor_offset = Box::<usize>::new(0);
    let _version = get_uint32_from_buf(&decoded[*sensor_offset..]);
    *sensor_offset += 4;
    let sensorsize = get_uint32_from_buf(&decoded[*sensor_offset..]);
    *sensor_offset += 4;           
    for _ in 0..sensorsize {
        let sensor = parse_sensor(&decoded, &mut sensor_offset);
        class_stream.push_str(&format!("{:?}", &sensor)[..]);
    }
}

pub fn deserialize_class(buf : &[u8], class_stream : &mut String, offset : &mut usize) {
    let classid = get_string_from_buf(&buf[*offset..]);
    println!("classid {:?}", classid);
    if classid == "" {
        println!("empty class id {:?}", get_stream_from_buf(&buf[*offset..]));
        return;
    }
    *offset += 4 + classid.len();
    if *offset == buf.len() {
        return;
    }
    match &classid[..] {
        "onServerList" => {
            let sizelist = get_uint32_from_buf(&buf[*offset..]);
            *offset += 4;
            for _ in 0..sizelist {
                deserialize_class(&buf, class_stream, offset);
            }
            }
        "BServerStatus" => { 
            let bsstatus = read_server_status(&buf, offset);
            class_stream.push_str(&format!("{:?}", &bsstatus)[..]);
        }
        "onGatewayList" => {
            //println!("{:?}", get_stream_from_buf(&buf[*offset..]));
            let _servername = get_stream_from_buf(&buf[*offset..*offset + 4]);
            *offset+=4;
            let sizelist = get_uint32_from_buf(&buf[*offset..]);
            *offset += 4;
            for _ in 0..sizelist {
                deserialize_class(&buf, class_stream, offset); 
            }   
        }
        "onRobotList" => {
            let _servername = get_stream_from_buf(&buf[*offset..*offset + 4]);
            *offset+=4;
            let sizelist = get_uint32_from_buf(&buf[*offset..]);
            *offset += 4;
            for _ in 0..sizelist {
                deserialize_class(&buf, class_stream, offset); 
            }   
        }
        "BRobotStatus" => {
            let brstatus = read_robot_status(&buf, offset);
            //println!("{:?}", brstatus);
            class_stream.push_str(&format!("{:?}", &brstatus)[..]);
        }
        "onSensors" => {
            parse_sensors(buf, class_stream, offset);
        }
        "onDaemonConfig" => {
            let bdstatus = read_daemon_status(&buf, offset);
            class_stream.push_str(&format!("{:?}", &bdstatus)[..]);
        }
        "onIndicator" => {
            let bindicator = read_indicator(&buf, offset);
            class_stream.push_str(&format!("{:?}", &bindicator));
        }
        _ => { class_stream.push_str(&classid); },
        }    
}

#[derive(PartialEq, Debug)] 
pub struct BSensor {
    category : Option<String>,
    rawName : Option<String>,
    dVal : Option<f64>,
    sVal : Option<String>,
    count : Option<u32>,
    min : Option<f64>,
    max : Option<f64>,
    total : Option<f64>,
    moment : Option<u64>,
}

pub fn parse_sensor(buf : &[u8], offset : &mut usize) -> BSensor {
    let mut bsensor = BSensor {
        category : None, 
        rawName : None, 
        dVal : None, 
        sVal : None, 
        count : None, 
        min : None, 
        max : None, 
        total : None, 
        moment : None};

    bsensor.category = Some(get_string_from_buf(&buf[*offset..]));

    *offset += 4 + bsensor.category.as_ref().unwrap().len();
    bsensor.rawName = Some(get_string_from_buf(&buf[*offset..]));
    *offset += 4 + bsensor.rawName.as_ref().unwrap().len();
    bsensor.dVal = Some(get_f64_from_buf(&buf[*offset..]));
    *offset += 8;
    bsensor.sVal = Some(get_string_from_buf(&buf[*offset..]));
    *offset += 4 + bsensor.sVal.as_ref().unwrap().len();
    bsensor.count = Some(get_uint32_from_buf(&buf[*offset..]));
    *offset += 4;
    bsensor.min = Some(get_f64_from_buf(&buf[*offset..]));
    *offset += 8;
    bsensor.max = Some(get_f64_from_buf(&buf[*offset..]));
    *offset += 8;
    bsensor.total = Some(get_f64_from_buf(&buf[*offset..]));
    *offset += 8; 
    bsensor.moment = Some(get_uint64_from_buf(&buf[*offset..]));
    *offset += 8; 
    println!("{:?}", bsensor); 
    bsensor
}

pub fn decode_sensors(buf : &[u8]) -> Vec<u8> {
    let mut decoder = Decoder::new(buf).unwrap();
    let mut decoded_data = Vec::new();
    decoder.read_to_end(&mut decoded_data).unwrap();
    decoded_data
}

pub fn deserialize_message(buf : &[u8]) -> BMessage {
    let mh = deserialize_header(&buf);
    let bsize = mh.bodySize as usize;
    let mut bmess = BMessage{mheader : mh, mpayload : None};
    let mut class_stream = String::new();
    let mut offset = Box::<usize>::new(0);
    if bsize > 0 {
        deserialize_class(&buf[10..bsize + 10], &mut class_stream, &mut offset);
        let param_vect : Vec<Box<dyn BObject>> = Vec::new();
        bmess.mpayload = Some(PayLoad{method_id : class_stream, params : param_vect});
    }
    bmess
}

pub fn pongMessage(tcpstream : &mut TcpStream) {
    let mut hsize = 10;
    let mut size = 0;
    let bitor = (FLAG_ENUM::FLAG_SNAPPY_SUPPORTED as u16| FLAG_ENUM::FLAG_KEEP_ALIVE_PONG as u16);
    let mheader = MessageHeader{flags: bitor, bodySize: 0, marker: MARKER_ENUM::MARKER_ONE_WAY as u32 };
    let mut buf_write = vec![0u8; hsize];
    let buf_read = vec![0u8; 1024];
    size += serialize_header_to_buf(&mheader, &mut buf_write);
    //println!("{:?}", deserialize_message(&buf_write));
    tcpstream.write(&buf_write[..size]);
}

pub fn storedSalt(username : &str, tcpstream : &mut TcpStream) -> u64 {
    let mut hsize = 10;
    let mut size = hsize;
    let mut mheader = MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 1};
    let mut buf_message = vec![0u8; 1024];
    size += serialize_to_buf("storedSalt".as_bytes(), &mut buf_message[size..], BObjectType::String);
    size += serialize_to_buf(username.as_bytes(), &mut buf_message[size ..], BObjectType::String);
    mheader.bodySize = (size - 10) as u32;
    hsize = serialize_header_to_buf(&mheader, &mut buf_message);
    //println!("{:?}", deserialize_message(&buf_message));
    tcpstream.write(&buf_message[..size]);
    tcpstream.read(&mut buf_message);
    get_uint64_from_buf(&buf_message[10..])
}

pub fn hashPassword(password: &str, salt : u64) -> String {
    let salt_bytes = salt.to_le_bytes();
    let salt_bytes_ptr : *const c_uchar = &salt_bytes[0];
    let mut hash = [0u8; openssl_sys::EVP_MAX_MD_SIZE as usize];
    let hash_ptr : *mut u8 = &mut hash[0];
    let saltlen : c_int = 8;
    let passlen : c_int = password.len() as c_int;
    let keylen : c_int = 32;
    let pass = CString::new(password).unwrap();
    let pass_ptr = pass.as_ptr();
    unsafe {
    openssl_sys::PKCS5_PBKDF2_HMAC(pass_ptr, passlen, salt_bytes_ptr, saltlen, openssl_sys::PKCS12_DEFAULT_ITER, openssl_sys::EVP_sha256(), keylen, hash_ptr);
    } 
    let mut hexstr = String::new();
    for i in 0..32 {
        hexstr.push_str(&format!("{:02x}", hash[i]));
    }
    hexstr
}

pub fn listen_bender(mut class_stream : &mut String,  mut tcpstream : &mut TcpStream,  parser: &dyn Fn(&[u8], &mut String, &mut usize)) {
    let mut bender_buffer = vec![0u8; 4096];
    let mut chunk_vector = Vec::<u8>::new();
    let mut counter = 0;
    let mut body_size = 0; 
    let mut message_offset = 0;
    let mut result;
    let mut hasheader = 1;
    while counter < 3 {   
        result = tcpstream.read(&mut bender_buffer).unwrap();
        //println!("received {:?} ping_messages {:?}", result, counter);
        //println!("stream: {:?}", get_stream_from_buf(&bender_buffer[0..result]));

        while counter < 3 {
            if body_size == 0 {
                if message_offset >= bender_buffer.len() {
                    message_offset = 0;
                    break;
            }   
            let header = deserialize_header(&bender_buffer[message_offset..result]);
        
            if (header.flags & (FLAG_ENUM::FLAG_KEEP_ALIVE_PING as u16)) !=0 {
                println!("ping:  {:?}", header);
                //pongMessage(&mut tcpstream);
                counter += 1;
                break;
            }
            
            body_size = header.bodySize as usize;
            if body_size == 0 {
                println!("empty body:  {:?}", header);
                counter+=1;
                break;
            }
                hasheader = 1;
            }

            if (message_offset + body_size + 10 * hasheader <= result) {
                let mut offset = Box::<usize>::new(0);
                chunk_vector.extend_from_slice(&bender_buffer[message_offset + 10 * hasheader ..message_offset + body_size + 10 * hasheader]);
                parser(&chunk_vector, &mut class_stream, &mut offset);
                
                let mut allprocessed = false;
                if message_offset + body_size + 10 * hasheader == result {
                    allprocessed = true;
                }
                chunk_vector.clear();
                message_offset += message_offset + body_size + 10 * hasheader;
                body_size = 0;
                if allprocessed {
                    body_size = 0;
                    hasheader = 1;
                    message_offset = 0;
                    break;
                }
                continue;
            }else {
                if result <= message_offset + 10 * hasheader {
                    return;
                }
                chunk_vector.extend_from_slice(&bender_buffer[message_offset + 10 * hasheader..result]);
                body_size -= result - message_offset - 10 * hasheader;
                message_offset = 0;
                hasheader = 0;
                break;
            }     
     }
     if counter > 3 {
        //pongMessage(&mut tcpstream);
        break;
     }
 }
}

pub fn encodeBase64(data : &str) -> u64 {
    let variable = base64::encode(data.as_bytes());
    let mut arr = [0u8; 8];
    arr.clone_from_slice(&variable.as_bytes()[0..8]);
    u64::from_le_bytes(arr)
}

pub fn decodeBase64(data : &str) -> String {
    let bytes : Vec::<u8> = base64::decode(data).unwrap();
    String::from_utf8(bytes).unwrap()
}

pub fn bmethod1(param1 : &str, param2 : u32, tcpstream : &mut TcpStream) {
    let mut size = 1024;
    let mut buf_message = vec![0u8; size];
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 3},   
    mpayload : Some(PayLoad{method_id : String::from(param1), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("bmethod1 {:?}", bms);
    println!("{:?}", &buf_message[..size]);
    tcpstream.write(&buf_message[..size]);  
}

pub fn bmethod2(param1 : &str, param2 : u32, param3 : &str, tcpstream : &mut TcpStream) {
    let mut size = 1024;
    let mut buf_message = vec![0u8; size];
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2));
    param_vec.push(Box::new(String::from(param3)));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 3},   
    mpayload : Some(PayLoad{method_id : String::from(param1), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("bmethod2 {:?}", bms);
    println!("{:?}", &buf_message[..size]);
    tcpstream.write(&buf_message[..size]);  
}

pub fn bmethod22(param1 : &str, param2 : u32, param3 : u64, tcpstream : &mut TcpStream) {
    let mut size = 1024;
    let mut buf_message = vec![0u8; size];
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2));
    param_vec.push(Box::new(param3));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 3},   
    mpayload : Some(PayLoad{method_id : String::from(param1), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("bmethod2 {:?}", bms);
    println!("{:?}", &buf_message[..size]);
    tcpstream.write(&buf_message[..size]);  
}

pub fn bmethod3(param1 : &str, param2 : u32, param3 : u64, param4 : f64, tcpstream : &mut TcpStream) {
    let mut size = 1024;
    let mut buf_message = vec![0u8; size];
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2));
    param_vec.push(Box::new(param3));
    param_vec.push(Box::new(param4));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 3},   
    mpayload : Some(PayLoad{method_id : String::from(param1), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("bmethod2 {:?}", bms);
    println!("{:?}", &buf_message[..size]);
    tcpstream.write(&buf_message[..size]);  
}

pub fn bmethod33(param1 : &str, param2 : u64, param3 : u64, param4 : f64, tcpstream : &mut TcpStream) {
    let mut size = 1024;
    let mut buf_message = vec![0u8; size];
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(param2));
    param_vec.push(Box::new(param3));
    param_vec.push(Box::new(param4));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 3},   
    mpayload : Some(PayLoad{method_id : String::from(param1), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("bmethod2 {:?}", bms);
    println!("{:?}", &buf_message[..size]);
    tcpstream.write(&buf_message[..size]);  
}

pub fn login(username : &str, password: &str, mut tcpstream : &mut TcpStream) -> BMessage {
    let mut buf_message = vec![0u8; 1024];
    let salt = storedSalt(username, &mut tcpstream);
    let hash = hashPassword(password, salt);
    let mut size = 1024;
    let mut param_vec : Vec<Box<dyn BObject>> = Vec::new();
    param_vec.push(Box::new(String::from(username)));
    param_vec.push(Box::new(String::from(String::from(hash))));
    let mut bms = BMessage{mheader : MessageHeader{flags: FLAG_ENUM::FLAG_METHOD as u16, bodySize: 0, marker: 2}, 
        mpayload : Some(PayLoad{method_id : String::from("login"), params : param_vec})};
    size = serialize_message(&mut bms, &mut buf_message);
    println!("login message: {:#?}", bms);

    tcpstream.write(&buf_message[..size]);
    tcpstream.read(&mut buf_message);
    
    //let header = deserialize_header(&buf_message[0..10]);
    //let bsize = header.bodySize as usize ;
    //let mut buf_message2 = vec![0u8; bsize];
    //let mut offset = Box::<usize>::new(0);
    //tcpstream.read(&mut buf_message2[0..bsize]);
    //let mut class_stream = String::new();
    //deserialize_class(&buf_message2, &mut class_strea
     deserialize_message(&buf_message)
}

pub fn encodeString(server_name : &str) -> u32 {
    let mut arr = [0u8; 4];
    arr.clone_from_slice(&server_name.as_bytes()[0..4]);
    u32::from_le_bytes(arr)
}

pub fn connect(server : &str, port : &str, buf : &mut [u8]) -> TcpStream {
    let mut session_name : String = String::from(server);
    session_name.push(':');
    session_name.push_str(port);
    println!("connecting: {:?}", session_name);
    let mut tcpstream = TcpStream::connect(&session_name).unwrap();
    &tcpstream.read(buf);  
    pongMessage(&mut tcpstream);
    tcpstream
}

/*pub fn  login("msamsonov", "7SqiiFUNWM5NrMirSHHh", &mut tcpstream) {
    //bmethod("statusGateways", encodeString("TND1"), ".*", &mut tcpstream);
  
    bmethod("gatewayConfig", encodeString("TND1"), "Binance.PROD", &mut tcpstream);

    let mut class_stream = String::new();
    listen_bender(&mut class_stream,  &mut tcpstream); 

    Ok(0)
} */
