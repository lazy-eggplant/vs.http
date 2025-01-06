#include "symbols.hpp"
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <vs-templ.hpp>
#include <mongoose.h>

extern "C"{
#include <canfigger.h>
}

struct global_t{
  std::string public_dir = "./public";            //Location of files to be served as is (resources)
  std::string public_prefix = "/public";          //Prefix path for public files
  std::string templates_dir = "./templates";      //Folder where templates are stored
  std::string src_dir = "./src";                  //Folder where static data sources are located.

  std::string http_url = "http://0.0.0.0:8500";
  std::string https_url = "";

  bool debug = true;
}globals;

struct mg_xml_writer : pugi::xml_writer {
  private:
    mg_connection* con;
    const char* ct;
  public:
    mg_xml_writer(mg_connection* _con, const char *_ct):con(_con),ct(_ct){}
    virtual ~mg_xml_writer(){}

		virtual void write(const void* data, size_t size) {
      mg_printf(
        con, 
        "HTTP/1.1 200 OK\r\n"
        "Cache-Control: no-cache\r\n"
        //"Content-Length: %d\r\n"
        "Content-Type: %s; charset=utf-8\r\n", ct);
      mg_send(con, "\r\n", 2);
      mg_send(con, data, size);
      con->is_resp = 0;
      con->is_draining = 1;
    }
};

//Utility function to avoid wasting time with strlen for const strings.
#define mg_sv( str) {(char*)str,sizeof(str)-1}

void logfn(vs::templ::log_t::values type, const char* msg, const vs::templ::logctx_t&){
    static const char* severity_table[] = {
    "\033[31;1m[ERROR]\033[0m    : ",
    "\033[33;1m[WARNING]\033[0m  : ",
    "\033[41;30;1m[PANIC]\033[0m    : ",
    "\033[34;1m[INFO]\033[0m     : ",
    };
    //TODO show context information
    std::cerr<<std::format("{}{} \033[33;1m@\033[0m {}",severity_table[type],msg,"xxx")<<"\n";
}

std::vector<std::string_view> split_string (std::string_view str, char delim) {
    std::vector<std::string_view> result;
    size_t i = 0, last = 0;
    for(;i<str.length(); i++){
        if(str[i]==delim){result.emplace_back(str.begin()+last,i-last);i++;last=i;}
    }
    result.emplace_back(str.begin()+last,i-last);
    return result;
}

bool handle_xml_file(mg_connection *c, mg_http_message *hm, pugi::xml_document& data_xml, std::map<std::string,vs::templ::symbol>& query){
  auto _template_filename = data_xml.root().first_child().attribute("template").as_string(nullptr);

  if(strcmp(data_xml.root().first_child().name(),"static-data")==0 && _template_filename!=nullptr){
    std::string ns = "s:";
    std::string template_filename=std::format("{}/{}",globals.templates_dir,_template_filename);
    pugi::xml_document template_xml;
    auto ret = template_xml.load_file(template_filename.c_str());

    //This would expose top level args as environment variables. Not sure if I want to have that.
    /*for(auto& prop : data_xml.root().first_child().attributes()){
      if(strcmp(prop.name(),"template")){
      }
      else{
        query.try_emplace(prop.name(),prop.value());
      }
    }*/

    if(ret.status==pugi::status_ok){

      //Logic to allow templates renaming
      {
        for(auto& prop : template_xml.root().first_child().attributes()){
          if(strncmp(prop.name(), "xmlns:",sizeof("xmlns:")-1)==0){
            if(strcmp(prop.value(),"vs.templ")==0){ns =  std::string(prop.name()+6)+":";}
          }
          else if(strcmp(prop.name(),"xmlns")){
            if(strcmp(prop.value(),"vs.templ")==0){ns = "";}
          }
        }
      }
      vs::templ::preprocessor preprocessor(data_xml,template_xml,ns.c_str(),logfn,+[](const char *name, pugi::xml_document& ref){
        std::string template_filename=std::format("{}/{}",globals.templates_dir,name);
        auto ret =ref.load_file(template_filename.c_str());
        if(ret.status==pugi::status_ok)return true;
        return false;
      });

      //Create the environment map
      //TODO: escaping needed.

      preprocessor.load_env(query);
      
      auto& result = preprocessor.parse();
      mg_xml_writer wr(c, result.root().first_child().attribute("content-type").as_string("application/xml"));

      result.save(wr);
      return true;
    }
    else{
      mg_http_reply(c, 404, "", "Template not usable!");
      return false;
    }
  }
  else{
    mg_xml_writer wr(c, data_xml.root().first_child().attribute("content-type").as_string("application/xml"));
    data_xml.save(wr);
    return true;
  }   
}

// HTTP server event handler function
void ev_handler(mg_connection *c, int ev, void *ev_data) {
  static std::string path = std::format("{}={}",globals.public_prefix,globals.public_dir);
  static std::string public_path = std::format("{}#",globals.public_prefix);

  if (ev == MG_EV_HTTP_MSG) {
    mg_http_message *hm = (mg_http_message *) ev_data;

    if(mg_match(hm->uri, {(char*)public_path.c_str(),public_path.size()}, NULL)){
      mg_http_serve_opts opts = { .root_dir = path.c_str() };
      mg_http_serve_dir(c, hm, &opts);
    }
    else if(mg_match(hm->method,mg_sv("GET"),NULL)){
      std::map<std::string,vs::templ::concrete_symbol> query;
      {
        //TODO: rewrite as a linear parser, but for now this is easier to implement.
        auto pairs = split_string({hm->query.buf,hm->query.buf+hm->query.len},'&');
        for(auto& pair : pairs){
          auto pieces = split_string(pair, '=');
          if(pieces.size()>1)query.try_emplace(std::string(pieces[0]),std::string(pieces[1]));
          else query.try_emplace(std::string(pieces[0]),true);
        }
      }

      pugi::xml_document data_xml;
      
      //Sequence of attempts
      if(data_xml.load_file(std::format("{}{}",globals.src_dir, std::string_view(hm->uri.buf,hm->uri.len)).c_str()).status==pugi::status_ok){
        if(handle_xml_file(c, hm, data_xml, query))return;
      }
      if(data_xml.load_file(std::format("{}{}.xml",globals.src_dir, std::string_view(hm->uri.buf,hm->uri.len)).c_str()).status==pugi::status_ok){
        if(handle_xml_file(c, hm, data_xml, query))return;
      }
      if(data_xml.load_file(std::format("{}{}/index.xml",globals.src_dir, std::string_view(hm->uri.buf,hm->uri.len)).c_str()).status==pugi::status_ok){
        if(handle_xml_file(c, hm, data_xml, query))return;
      }

      mg_http_reply(c, 404, "", "File not found!");
      
    }
    else{
      mg_http_reply(c, 412, "", "Unable to process this request");
    }
  }
}

int main(int argc, const char* argv[]) {
  if(argc==2){
    const char *config_file = argv[1];
    //TODO: fill in `globals` based on its content.

    Canfigger *config = canfigger_parse_file(config_file, ',');
    if(!config){
      std::cerr<<"Unable to parse configuration file passed\n";
    }
    while (config != NULL){
      if(false){}
      else if(strcmp(config->key,"public_dir")==0 && config->value!=nullptr)globals.public_dir=config->value; 
      else if(strcmp(config->key,"public_prefix")==0 && config->value!=nullptr)globals.public_prefix=config->value; 
      else if(strcmp(config->key,"templates_dir")==0 && config->value!=nullptr)globals.templates_dir=config->value; 
      else if(strcmp(config->key,"src_dir")==0 && config->value!=nullptr)globals.src_dir=config->value; 
      else if(strcmp(config->key,"http_url")==0 && config->value!=nullptr)globals.http_url=config->value; 
      else if(strcmp(config->key,"https_url")==0 && config->value!=nullptr)globals.https_url=config->value; 
      else if(strcmp(config->key,"debug")==0 && config->value!=nullptr){
        if(strcmp(config->key,"true")==0)globals.debug=true; 
        else globals.debug=false;
      }
      else{
        std::cerr<<std::format("\033[33;1m[WARNING]\033[0m  : Configuration property `{}` not recognized \033[33;1m@\033[0m", config->key)<<"\n";
      }
      canfigger_free_current_key_node_advance(&config);
    }
  }

  mg_mgr mgr;  // Declare event manager
  if(globals.debug)mg_log_set(MG_LL_DEBUG);
  mg_mgr_init(&mgr);  // Initialise event manager
  if(globals.http_url!="")mg_http_listen(&mgr, globals.http_url.c_str(), ev_handler, NULL); 
  if(globals.https_url!="")mg_http_listen(&mgr, globals.https_url.c_str(), ev_handler, NULL); 

  for (;;) {          // Run an infinite event loop
    mg_mgr_poll(&mgr, 1000);
  }

  mg_mgr_free(&mgr);
  return 0;
}