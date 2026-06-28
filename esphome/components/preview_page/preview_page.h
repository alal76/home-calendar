// ────────────────────────────────────────────────────────────────
//  preview_page.h
//  Registers GET /preview on the existing ESPHome web server.
//  Returns an HTML+SVG dashboard that mirrors the e-paper layout,
//  using the live event globals (ev_count / ev_titles / ev_starts /
//  ev_ends / ev_all_day / last_sync / last_status) populated by the
//  calendar text_sensor in esp32connector.yaml.
// ────────────────────────────────────────────────────────────────
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <array>
#include <cstdio>
#include <string>

namespace esphome
{
  namespace preview_page
  {

    class PreviewPage : public Component, public AsyncWebHandler
    {
    public:
      explicit PreviewPage(web_server_base::WebServerBase *base) : base_(base) {}

      void setup() override
      {
        // Register the HTTP handler. IDF's web_server iterates the
        // handlers list in insertion order and the first canHandle() that
        // returns true wins, so we must register BEFORE the web_server
        // component does — get_setup_priority() below is set higher than
        // setup_priority::WIFI (the web_server's priority) to guarantee
        // that ordering.
        this->base_->add_handler(this);
      }
      float get_setup_priority() const override
      {
        return setup_priority::PROCESSOR;
      }

      // Wired in from yaml on_boot: pointers to the globals.
      void set_count(int *p) { this->count_ = p; }
      void set_titles(std::array<std::string, 96> *p) { this->titles_ = p; }
      void set_starts(std::array<std::string, 96> *p) { this->starts_ = p; }
      void set_ends(std::array<std::string, 96> *p) { this->ends_ = p; }
      void set_all_day(std::array<bool, 96> *p) { this->all_day_ = p; }
      void set_last_sync(std::string *p) { this->last_sync_ = p; }
      void set_status(std::string *p) { this->status_ = p; }
      void set_weather_state(text_sensor::TextSensor *p) { this->wx_state_ = p; }
      void set_weather_temp(text_sensor::TextSensor *p) { this->wx_temp_ = p; }
      void set_month_offset(int *p) { this->month_offset_ = p; }
      void set_person_a(text_sensor::TextSensor *p) { this->person_a_ = p; }
      void set_person_b(text_sensor::TextSensor *p) { this->person_b_ = p; }
      void set_person_c(text_sensor::TextSensor *p) { this->person_c_ = p; }
      void set_person_d(text_sensor::TextSensor *p) { this->person_d_ = p; }
      void set_speed_a(text_sensor::TextSensor *p) { this->speed_a_ = p; }
      void set_speed_b(text_sensor::TextSensor *p) { this->speed_b_ = p; }
      void set_speed_c(text_sensor::TextSensor *p) { this->speed_c_ = p; }
      void set_speed_d(text_sensor::TextSensor *p) { this->speed_d_ = p; }
      void set_person_labels(const std::string &a, const std::string &b,
                             const std::string &c, const std::string &d)
      {
        this->person_a_label_ = a;
        this->person_b_label_ = b;
        this->person_c_label_ = c;
        this->person_d_label_ = d;
      }
      void set_sw_version(const std::string &v) { this->sw_version_ = v; }

      bool canHandle(AsyncWebServerRequest *request) const override
      {
        if (request->method() != HTTP_GET)
          return false;
        char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
        auto u = request->url_to(url_buf);
        if (u == "/preview" || u == "/dash")
          return true;
        // Claim "/" only when the caller did NOT add ?dash=1.
        if (u == "/" && !request->hasParam("dash"))
          return true;
        return false;
      }

      void handleRequest(AsyncWebServerRequest *request) override;

    private:
      web_server_base::WebServerBase *base_;
      int *count_{nullptr};
      std::array<std::string, 96> *titles_{nullptr};
      std::array<std::string, 96> *starts_{nullptr};
      std::array<std::string, 96> *ends_{nullptr};
      std::array<bool, 96> *all_day_{nullptr};
      std::string *last_sync_{nullptr};
      std::string *status_{nullptr};
      text_sensor::TextSensor *wx_state_{nullptr};
      text_sensor::TextSensor *wx_temp_{nullptr};
      int *month_offset_{nullptr};
      text_sensor::TextSensor *person_a_{nullptr};
      text_sensor::TextSensor *person_b_{nullptr};
      text_sensor::TextSensor *person_c_{nullptr};
      text_sensor::TextSensor *person_d_{nullptr};
      text_sensor::TextSensor *speed_a_{nullptr};
      text_sensor::TextSensor *speed_b_{nullptr};
      text_sensor::TextSensor *speed_c_{nullptr};
      text_sensor::TextSensor *speed_d_{nullptr};
      std::string person_a_label_{"A"};
      std::string person_b_label_{"B"};
      std::string person_c_label_{"C"};
      std::string person_d_label_{"D"};
      std::string sw_version_{""};
    };

    // JSON string escaping for embedding into the inline <script>.
    inline std::string js_escape(const std::string &s)
    {
      std::string out;
      out.reserve(s.size() + 4);
      for (char c : s)
      {
        switch (c)
        {
        case '\\':
          out += "\\\\";
          break;
        case '"':
          out += "\\\"";
          break;
        case '\n':
          out += "\\n";
          break;
        case '\r':
          out += "\\r";
          break;
        case '\t':
          out += "\\t";
          break;
        case '<':
          out += "\\u003c";
          break;
        case '>':
          out += "\\u003e";
          break;
        case '&':
          out += "\\u0026";
          break;
        default:
          if (static_cast<unsigned char>(c) < 0x20)
          {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
          }
          else
          {
            out += c;
          }
        }
      }
      return out;
    }

    inline void PreviewPage::handleRequest(AsyncWebServerRequest *request)
    {
      // /dash → meta-refresh to "/?dash=1". The IDF beginResponse helper
      // only knows status codes 200/401/404 so a real 302 falls back to
      // 500; meta-refresh sidesteps that.
      char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
      if (request->url_to(url_buf) == "/dash")
      {
        request->send(200, "text/html",
                      "<!doctype html><meta http-equiv=\"refresh\" "
                      "content=\"0;url=/?dash=1\">"
                      "<a href=\"/?dash=1\">Continue →</a>");
        return;
      }

      // Build the events JSON literal from the live globals.
      std::string json = "[";
      const int n = this->count_ ? *this->count_ : 0;
      if (this->titles_ && this->starts_ && this->all_day_)
      {
        for (int i = 0; i < n && i < 96; i++)
        {
          if (i)
            json += ",";
          const std::string end_s =
              (this->ends_ ? (*this->ends_)[i] : std::string());
          json += "{\"start\":\"" + js_escape((*this->starts_)[i]) +
                  "\",\"end\":\"" + js_escape(end_s) +
                  "\",\"summary\":\"" + js_escape((*this->titles_)[i]) +
                  "\",\"all_day\":" +
                  ((*this->all_day_)[i] ? "true" : "false") + "}";
        }
      }
      json += "]";

      const std::string last_sync = this->last_sync_ ? *this->last_sync_ : "never";
      const std::string status = this->status_ ? *this->status_ : "?";
      const int month_offset_val = this->month_offset_ ? *this->month_offset_ : 0;

      // Weather (HA Met.no built-in). text_sensors may be missing if HA
      // hasn't published yet; guard for nullptr + has_state.
      std::string weather_state_str, weather_temp_str;
      if (this->wx_state_ && this->wx_state_->has_state())
        weather_state_str = this->wx_state_->state;
      if (this->wx_temp_ && this->wx_temp_->has_state())
        weather_temp_str = this->wx_temp_->state;

      // People (HA Companion App). Same guard pattern. Speed sensors
      // are off by default and may stay "" forever.
      auto ts_val = [](text_sensor::TextSensor *t) -> std::string
      {
        if (!t || !t->has_state())
          return "";
        const std::string &s = t->state;
        if (s == "unavailable" || s == "unknown")
          return "";
        return s;
      };
      const std::string person_a_st = ts_val(this->person_a_);
      const std::string person_b_st = ts_val(this->person_b_);
      const std::string person_c_st = ts_val(this->person_c_);
      const std::string person_d_st = ts_val(this->person_d_);
      const std::string speed_a_st = ts_val(this->speed_a_);
      const std::string speed_b_st = ts_val(this->speed_b_);
      const std::string speed_c_st = ts_val(this->speed_c_);
      const std::string speed_d_st = ts_val(this->speed_d_);

      std::string html;
      html.reserve(16384);

      // ── page chrome ──
      html +=
          "<!doctype html><html><head><meta charset=\"utf-8\">"
          "<meta name=\"viewport\" content=\"width=832\">"
          "<title>esp32connector — calendar preview</title>"
          "<style>"
          "body{margin:0;background:#1a1a1a;color:#ccc;"
          "font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;"
          "display:flex;flex-direction:column;align-items:center;"
          "padding:14px;gap:10px;}"
          ".meta{font-size:12px;color:#888;text-align:center;}"
          ".meta b{color:#eee;} .meta a{color:#9cf;text-decoration:none;}"
          ".frame{width:800px;height:480px;background:#fff;"
          "box-shadow:0 6px 40px rgba(0,0,0,.6);border:1px solid #444;}"
          "svg{display:block;}"
          "button{background:#2a2a2a;color:#eee;border:1px solid #555;"
          "border-radius:6px;padding:8px 18px;font-size:13px;cursor:pointer;}"
          "button:hover{background:#3a3a3a;border-color:#888;}"
          "button:disabled{opacity:.5;cursor:wait;}"
          ".card{width:800px;background:#222;border:1px solid #444;"
          "border-radius:8px;padding:14px 18px;font-size:13px;"
          "display:grid;grid-template-columns:1fr 1fr;gap:6px 24px;}"
          ".card h3{grid-column:1/-1;margin:0 0 4px;color:#eee;font-size:13px;"
          "letter-spacing:.08em;text-transform:uppercase;}"
          ".card .k{color:#888;} .card .v{color:#eee;text-align:right;}"
          ".card .full{grid-column:1/-1;color:#aaa;font-size:12px;}"
          ".nav{display:flex;gap:8px;align-items:center;}"
          ".nav .ib{background:#2a2a2a;color:#eee;border:1px solid #555;"
          "border-radius:6px;width:42px;height:36px;font-size:18px;"
          "line-height:1;cursor:pointer;display:inline-flex;"
          "align-items:center;justify-content:center;}"
          ".nav .ib:hover{background:#3a3a3a;border-color:#888;}"
          ".nav .ib:disabled{opacity:.5;cursor:wait;}"
          ".nav .lbl{color:#888;font-size:12px;min-width:90px;text-align:center;}"
          ".nav .lbl b{color:#eee;}"
          "</style></head><body>"
          "<div class=\"meta\">esp32connector live preview · "
          "<a href=\"/preview\">reload</a> · "
          "<a href=\"/dash\">entities dashboard →</a></div>"
          "<div class=\"nav\">"
          "<button class=\"ib\" id=\"nv-prev\" title=\"Previous month\" "
          "onclick=\"navMonth('prev_month')\">\u25c0</button>"
          "<button class=\"ib\" id=\"nv-today\" title=\"Back to current month\" "
          "onclick=\"navMonth('reset_month')\">\u2299</button>"
          "<button class=\"ib\" id=\"nv-next\" title=\"Next month\" "
          "onclick=\"navMonth('next_month')\">\u25b6</button>"
          "<span class=\"lbl\" id=\"nv-lbl\">…</span>"
          "</div>"
          "<div class=\"frame\"><svg id=\"d\" width=\"800\" height=\"480\" "
          "viewBox=\"0 0 800 480\" xmlns=\"http://www.w3.org/2000/svg\" "
          "font-family=\"Roboto,Helvetica,Arial,sans-serif\"></svg></div>"
          "<button id=\"rf\" onclick=\"refreshCal()\">Refresh Calendar</button>"
          "<div class=\"card\" id=\"info\">"
          "<h3>Device</h3>"
          "<span class=\"k\">Hostname</span><span class=\"v\" id=\"i-host\">…</span>"
          "<span class=\"k\">IP</span><span class=\"v\" id=\"i-ip\">…</span>"
          "<span class=\"k\">Uptime</span><span class=\"v\" id=\"i-up\">…</span>"
          "<span class=\"k\">WiFi RSSI</span><span class=\"v\" id=\"i-rssi\">…</span>"
          "<span class=\"k\">Events loaded</span><span class=\"v\" id=\"i-evt\">…</span>"
          "<span class=\"k\">Weather</span><span class=\"v\" id=\"i-wx\">…</span>"
          "<span class=\"k\">Last sync</span><span class=\"v\" id=\"i-ls\">…</span>"
          "<span class=\"k\">Sync status</span><span class=\"v\" id=\"i-ss\">…</span>"
          "<span class=\"k\" id=\"i-pa-k\">Person A</span>"
          "<span class=\"v\" id=\"i-pa\">…</span>"
          "<span class=\"k\" id=\"i-pb-k\">Person B</span>"
          "<span class=\"v\" id=\"i-pb\">…</span>"
          "<span class=\"k\" id=\"i-pc-k\">Person C</span>"
          "<span class=\"v\" id=\"i-pc\">…</span>"
          "<span class=\"k\" id=\"i-pd-k\">Person D</span>"
          "<span class=\"v\" id=\"i-pd\">…</span>"
          "<span class=\"k\">Firmware</span><span class=\"v\" id=\"i-fw\">…</span>"
          "<span class=\"full\">Source: Home Assistant calendar.get_events "
          "(-7d to +90d window) via sensor.calendar_events_json, refreshed "
          "every 5 min.</span>"
          "</div>";

      // ── renderer (mirror of display lambda in esp32connector.yaml) ──
      html += "<script>const EVENTS=";
      html += json;
      html += ";const WX_STATE=\"";
      html += js_escape(weather_state_str);
      html += "\",WX_TEMP=\"";
      html += js_escape(weather_temp_str);
      html += "\",LAST_SYNC=\"";
      html += js_escape(last_sync);
      html += "\",STATUS=\"";
      html += js_escape(status);
      html += "\",MONTH_OFFSET=";
      html += std::to_string(month_offset_val);
      html += ",PA_LBL=\"";
      html += js_escape(this->person_a_label_);
      html += "\",PB_LBL=\"";
      html += js_escape(this->person_b_label_);
      html += "\",PC_LBL=\"";
      html += js_escape(this->person_c_label_);
      html += "\",PD_LBL=\"";
      html += js_escape(this->person_d_label_);
      html += "\",PA_ST=\"";
      html += js_escape(person_a_st);
      html += "\",PB_ST=\"";
      html += js_escape(person_b_st);
      html += "\",PC_ST=\"";
      html += js_escape(person_c_st);
      html += "\",PD_ST=\"";
      html += js_escape(person_d_st);
      html += "\",PA_SP=\"";
      html += js_escape(speed_a_st);
      html += "\",PB_SP=\"";
      html += js_escape(speed_b_st);
      html += "\",PC_SP=\"";
      html += js_escape(speed_c_st);
      html += "\",PD_SP=\"";
      html += js_escape(speed_d_st);
      html += "\",SW_VER=\"";
      html += js_escape(this->sw_version_);
      html += "\";";

      html +=
          // constants
          "const W=800,H=480,CAL_W=568,EVT_X=568,EVT_W=232,"
          "HDR_H=50,GRID_TOP=78,STATUS_Y=400,STATUS_H=80;"
          "const RED='#d70000',BLK='#111',WHT='#fff';"
          "const svg=document.getElementById('d'),"
          "NS='http://www.w3.org/2000/svg';"
          "function el(t,a,x){const e=document.createElementNS(NS,t);"
          "for(const k in a)e.setAttribute(k,a[k]);"
          "if(x!=null)e.textContent=x;svg.appendChild(e);return e;}"
          "const rect=(x,y,w,h,f,s)=>el('rect',"
          "{x,y,width:w,height:h,fill:f,stroke:s||'none'});"
          "const line=(a,b,c,d,col)=>el('line',"
          "{x1:a,y1:b,x2:c,y2:d,stroke:col||BLK,'stroke-width':1});"
          "const circ=(x,y,r,f)=>el('circle',{cx:x,cy:y,r:r,fill:f});"
          "function txt(x,y,sz,c,an,wt,t){el('text',"
          "{x,y,fill:c,'font-size':sz,'text-anchor':an||'start',"
          "'dominant-baseline':'hanging','font-weight':wt||'normal'},t);}"
          // weather label map (same set as the C++ lambda)
          "const WX_MAP={'clear-night':'Clear night','cloudy':'Cloudy',"
          "'exceptional':'Severe','fog':'Fog','hail':'Hail',"
          "'lightning':'Thunder','lightning-rainy':'Thunderstorm',"
          "'partlycloudy':'Partly cloudy','pouring':'Pouring','rainy':'Rain',"
          "'snowy':'Snow','snowy-rainy':'Sleet','sunny':'Sunny','windy':'Windy',"
          "'windy-variant':'Windy'};"
          "function wxLine(){let p='';"
          "if(WX_TEMP){const t=parseFloat(WX_TEMP);"
          "if(!isNaN(t))p=Math.round(t)+'°C';}"
          "if(WX_STATE){const nice=WX_MAP[WX_STATE]||WX_STATE;"
          "p=p?p+' · '+nice:nice;}return p;}"
          // date helpers
          "function pad2(n){return n<10?'0'+n:''+n;}"
          "function fmtDate(s){if(!s||s.length<10)return '';"
          "const y=+s.substring(0,4),m=+s.substring(5,7),d=+s.substring(8,10);"
          "const dt=new Date(y,m-1,d);"
          "const dn=['Sun','Mon','Tue','Wed','Thu','Fri','Sat'][dt.getDay()];"
          "const mn=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug',"
          "'Sep','Oct','Nov','Dec'][m-1];"
          "return dn+' '+d+' '+mn;}"
          "function tStr(s){return (!s||s.length<16)?''"
          ":s.substring(11,13)+':'+s.substring(14,16);}"
          "function dayKey(s){return s.substring(0,10);}"
          // ── today / rolling-window header ──
          "const now=new Date(),realMo=now.getMonth(),realYr=now.getFullYear(),"
          "td=now.getDate(),"
          "todayKey=realYr+'-'+pad2(realMo+1)+'-'+pad2(td);"
          // Rolling 5-week window: start = Monday of (anchor week − 2).
          // anchor = today shifted by MONTH_OFFSET × 28 days (≈ months).
          "const NUM_ROWS=5,NUM_CELLS=NUM_ROWS*7;"
          "const anchor=new Date(realYr,realMo,td+MONTH_OFFSET*28);"
          "const anchorDow=(anchor.getDay()+6)%7;" // Mon=0 … Sun=6
          "const startD=new Date(anchor.getFullYear(),anchor.getMonth(),"
          "anchor.getDate()-anchorDow-14);"
          "const endD=new Date(startD.getFullYear(),startD.getMonth(),"
          "startD.getDate()+NUM_CELLS-1);"
          "function dayAt(idx){"
          "return new Date(startD.getFullYear(),startD.getMonth(),"
          "startD.getDate()+idx);}"
          "function keyAt(idx){const d=dayAt(idx);"
          "return d.getFullYear()+'-'+pad2(d.getMonth()+1)+'-'+pad2(d.getDate());}"
          "function dateToCell(ymd){"
          "const y=+ymd.substring(0,4),m=+ymd.substring(5,7),"
          "d=+ymd.substring(8,10);"
          "const dt=new Date(y,m-1,d);"
          "const diff=Math.round((dt-startD)/86400000);"
          "return (diff<0||diff>=NUM_CELLS)?-1:diff;}"
          "const todayCell=dateToCell(todayKey);"
          // Header: month range, collapsing to single month when same.
          "rect(0,0,CAL_W,HDR_H,RED);"
          "const MONTH_NAMES=['Jan','Feb','Mar','Apr','May','Jun','Jul',"
          "'Aug','Sep','Oct','Nov','Dec'];"
          "const MONTH_FULL=['January','February','March','April','May','June',"
          "'July','August','September','October','November','December'];"
          "let hdrTxt;"
          "if(startD.getMonth()===endD.getMonth()&&"
          "startD.getFullYear()===endD.getFullYear())"
          "hdrTxt=MONTH_FULL[startD.getMonth()]+' '+startD.getFullYear();"
          "else if(startD.getFullYear()===endD.getFullYear())"
          "hdrTxt=MONTH_NAMES[startD.getMonth()]+' – '+"
          "MONTH_NAMES[endD.getMonth()]+' '+startD.getFullYear();"
          "else hdrTxt=MONTH_NAMES[startD.getMonth()]+' '+startD.getFullYear()+"
          "' – '+MONTH_NAMES[endD.getMonth()]+' '+endD.getFullYear();"
          "if(MONTH_OFFSET!==0)hdrTxt+='  ('+"
          "(MONTH_OFFSET>0?'+':'')+MONTH_OFFSET+'mo)';"
          "txt(16,10,28,WHT,'start','700',hdrTxt);"
          "rect(EVT_X,0,EVT_W,HDR_H,RED);"
          "txt(EVT_X+12,6,18,WHT,'start','700','Calendar');"
          "const wxl=wxLine();"
          "if(wxl)txt(EVT_X+12,28,11,WHT,'start','normal',wxl);"
          "line(CAL_W,0,CAL_W,STATUS_Y);"
          // day-of-week labels (weekend in red)
          "const days=['Mo','Tu','We','Th','Fr','Sa','Su'],colW=CAL_W/7;"
          "for(let i=0;i<7;i++)txt(i*colW+colW/2,56,12,"
          "i>=5?RED:BLK,'middle','normal',days[i]);"
          "line(0,74,CAL_W,74);"
          // grid math — 5 rows
          "const rowH=(STATUS_Y-GRID_TOP)/NUM_ROWS;"
          "const cellX=[],cellY=[];"
          // Pass 1: cells + day numbers + today + month-break label
          "for(let cell=0;cell<NUM_CELLS;cell++){"
          "const row=(cell/7)|0,col=cell%7;"
          "const x=col*colW,y=GRID_TOP+row*rowH;"
          "cellX[cell]=x;cellY[cell]=y;"
          "rect(x,y,colW,rowH,'none',BLK);"
          "const dt=dayAt(cell);"
          "const isToday=(cell===todayCell);"
          "if(isToday)circ(x+colW/2,y+18,15,RED);"
          "txt(x+colW/2,y+6,18,isToday?WHT:BLK,'middle','700',"
          "String(dt.getDate()));"
          "if(dt.getDate()===1)"
          "txt(x+colW-4,y+6,10,RED,'end','700',MONTH_NAMES[dt.getMonth()]);}"
          // Pass 2: multi-day bars (cell-index based)
          "const barSlots=Array.from({length:NUM_ROWS},()=>[0,0,0,0,0,0,0]);"
          "const BAR_BASE=24,BAR_H=11,BAR_GAP=1,MAX_BARS=2;"
          "function addDays(ymd,delta){"
          "const y=+ymd.substring(0,4),m=+ymd.substring(5,7),"
          "d=+ymd.substring(8,10);"
          "const dt=new Date(y,m-1,d+delta);"
          "return dt.getFullYear()+'-'+pad2(dt.getMonth()+1)+"
          "'-'+pad2(dt.getDate());}"
          "for(const ev of EVENTS){"
          "const s=(ev.start||'')+'',eo=(ev.end||'')+'';"
          "if(s.length<10||eo.length<10)continue;"
          "let endKey=dayKey(eo);"
          "if(ev.all_day||s.length===10)endKey=addDays(endKey,-1);"
          "const startKey=dayKey(s);"
          "if(startKey===endKey)continue;" // single day → dot pass
          "const sCell=dateToCell(startKey),eCell=dateToCell(endKey);"
          // need at least partial overlap with window
          "let sIdx=sCell,eIdx=eCell;"
          "if(sIdx===-1){"
          "if(startKey>keyAt(NUM_CELLS-1))continue;"
          "sIdx=0;}"
          "if(eIdx===-1){"
          "if(endKey<keyAt(0))continue;"
          "eIdx=NUM_CELLS-1;}"
          "if(eIdx<0||sIdx>=NUM_CELLS)continue;"
          "let firstSeg=(sCell>=0),d=sIdx;"
          "while(d<=eIdx){"
          "const r=(d/7)|0,c0=d%7;"
          "const maxInRow=Math.min(eIdx,d+6-c0);"
          "const wc=maxInRow-d+1;"
          "const slot=barSlots[r][c0];"
          "if(slot<MAX_BARS){"
          "const x1=c0*colW+2,x2=(c0+wc-1)*colW+colW-2;"
          "const yb=GRID_TOP+r*rowH+BAR_BASE+slot*(BAR_H+BAR_GAP);"
          "rect(x1,yb,x2-x1,BAR_H,RED);"
          "if(firstSeg){"
          "let t=ev.summary||'';"
          "const maxCh=Math.max(1,((x2-x1-4)/6)|0);"
          "if(t.length>maxCh)t=t.substring(0,Math.max(1,maxCh-1))+'.';"
          "txt(x1+3,yb+1,10,BLK,'start','normal',t);}"
          "for(let cc=c0;cc<c0+wc;cc++)barSlots[r][cc]++;}"
          "firstSeg=false;d=maxInRow+1;}}"
          // Pass 3: single-day event dots + first-event label per cell
          "for(let cell=0;cell<NUM_CELLS;cell++){"
          "const x=cellX[cell],y=cellY[cell];"
          "const cellKey=keyAt(cell);"
          "let dots=0,label='';"
          "for(const ev of EVENTS){if(dots>=3)break;"
          "const s=(ev.start||'')+'',eo=(ev.end||'')+'';"
          "if(s.length<10)continue;"
          "if(dayKey(s)!==cellKey)continue;"
          "if(eo.length>=10){"
          "let endKey=dayKey(eo);"
          "if(ev.all_day||s.length===10)endKey=addDays(endKey,-1);"
          "if(dayKey(s)!==endKey)continue;}"
          "circ(x+8+dots*12,y+rowH-9,4,cellKey<todayKey?BLK:RED);"
          "if(dots===0)label=ev.summary||'';dots++;}"
          "if(label){const lb=label.length>9?label.substring(0,9):label;"
          "txt(x+3,y+rowH-22,10,BLK,'start','normal',lb);}}"
          // ── sidebar: TODAY + COMING UP (up to 10 events) ──
          "const today_ev=[],upcoming=[];"
          "for(let i=0;i<EVENTS.length;i++){"
          "const s=(EVENTS[i].start||'')+'';if(s.length<10)continue;"
          "const k=dayKey(s);"
          "if(k===todayKey)today_ev.push(i);"
          "else if(k>todayKey)upcoming.push(i);}"
          "let py=HDR_H+6;const sx=EVT_X+8,rh=26,maxY=STATUS_Y-6,"
          "MAX_SIDE=10;"
          "let shownTotal=0;"
          "function drawEvent(idx,titleCol,dateCol){"
          "if(py+rh>maxY||shownTotal>=MAX_SIDE)return false;"
          "const ev=EVENTS[idx],s=(ev.start||'')+'';"
          "let dl=fmtDate(s);"
          "if(!ev.all_day&&s.length>=16)dl+=' '+tStr(s);"
          "else if(ev.all_day)dl+=' all-day';"
          "txt(sx,py,10,dateCol,'start','normal',dl);"
          "let t=ev.summary||'';if(t.length>28)t=t.substring(0,27)+'.';"
          "txt(sx,py+11,11,titleCol,'start','600',t);"
          "py+=rh;shownTotal++;return true;}"
          "function drawSub(label,col){"
          "if(py+14>maxY)return;"
          "txt(sx,py,10,col||BLK,'start','700',label);"
          "py+=14;}"
          "if(today_ev.length){drawSub('TODAY',RED);"
          "for(const idx of today_ev){"
          "if(!drawEvent(idx,BLK,RED))break;}"
          "py+=2;}"
          "if(upcoming.length&&shownTotal<MAX_SIDE){drawSub('COMING UP',BLK);"
          "for(const idx of upcoming){"
          "if(!drawEvent(idx,BLK,RED))break;}}"
          "if(!today_ev.length&&!upcoming.length)"
          "txt(sx,py+8,12,BLK,'start','normal','No events');"
          // ── 3-row footer ──
          "line(0,STATUS_Y,W,STATUS_Y);"
          "const tot=EVENTS.length;"
          "let tCount=0,uCount=0;"
          "for(const ev of EVENTS){const s=(ev.start||'')+'';"
          "if(s.length<10)continue;const k=dayKey(s);"
          "if(k===todayKey)tCount++;else if(k>todayKey)uCount++;}"
          "txt(8,STATUS_Y+6,11,BLK,'start','normal',"
          "'Updated: '+LAST_SYNC+'  ·  '+tot+' loaded  ·  '+"
          "tCount+' today  ·  '+uCount+' upcoming');"
          // next-event row
          "let next=null;"
          "for(const ev of EVENTS){const s=(ev.start||'')+'';"
          "if(s.length<10)continue;const k=dayKey(s);"
          "if(k>=todayKey){next=ev;break;}}"
          "if(next){let nl='Next: '+(next.summary||'')+' — '+"
          "fmtDate(next.start);"
          "if(!next.all_day&&next.start.length>=16)"
          "nl+=' at '+tStr(next.start);"
          "const nk=dayKey(next.start);"
          "if(nk===todayKey)nl+=' (today)';"
          "else{const a=new Date(realYr,realMo,td),"
          "b=new Date(+nk.substring(0,4),+nk.substring(5,7)-1,"
          "+nk.substring(8,10));"
          "const dd=Math.round((b-a)/86400000);"
          "nl+=' (in '+dd+'d)';}"
          "if(nl.length>96)nl=nl.substring(0,95)+'.';"
          "txt(8,STATUS_Y+30,11,BLK,'start','normal',nl);}"
          "else txt(8,STATUS_Y+30,11,BLK,'start','normal','Next: —');"
          // wifi/ip/uptime row — populated by JS fetch below
          "el('text',{x:8,y:STATUS_Y+54,id:'fwifi',fill:BLK,"
          "'font-size':11,'dominant-baseline':'hanging'},"
          "'WiFi: …');"
          "txt(W-8,STATUS_Y+54,11,BLK,'end','normal',"
          "'esp32connector'+(SW_VER?' v'+SW_VER:''));"
          // row 4: people + (optional) speed from HA Companion App
          "function fmtPerson(lbl,st,sp){"
          "let out=lbl+': '+(st?(st==='not_home'?'away':st):'?');"
          "if(sp){const m=parseFloat(sp);"
          "if(!isNaN(m)&&m>=1.0)out+=' ('+Math.round(m*3.6)+' km/h)';}"
          "return out;}"
          "const row4=fmtPerson(PA_LBL,PA_ST,PA_SP)+'  ·  '+"
          "fmtPerson(PB_LBL,PB_ST,PB_SP)+'  ·  '+"
          "fmtPerson(PC_LBL,PC_ST,PC_SP)+'  ·  '+"
          "fmtPerson(PD_LBL,PD_ST,PD_SP);"
          "txt(8,STATUS_Y+70,10,BLK,'start','normal',row4);"
          // ── refresh button + live device info fetch ──
          "function refreshCal(){"
          "const b=document.getElementById('rf');"
          "b.disabled=true;b.textContent='Refreshing...';"
          "fetch('/button/refresh_calendar/press',{method:'POST'})"
          ".then(r=>{b.textContent=r.ok?'Requested — reloading...':"
          "'Failed ('+r.status+')';"
          "if(r.ok)setTimeout(()=>location.reload(),3000);"
          "else b.disabled=false;})"
          ".catch(e=>{b.textContent='Error';b.disabled=false;});}"
          // ── month navigation (icon buttons) ──
          "function navMonth(slug){"
          "const ids=['nv-prev','nv-today','nv-next'];"
          "ids.forEach(i=>{const b=document.getElementById(i);"
          "if(b)b.disabled=true;});"
          "fetch('/button/'+slug+'/press',{method:'POST'})"
          ".then(r=>{if(r.ok){setTimeout(()=>location.reload(),900);}"
          "else{ids.forEach(i=>{const b=document.getElementById(i);"
          "if(b)b.disabled=false;});}})"
          ".catch(()=>{ids.forEach(i=>{const b=document.getElementById(i);"
          "if(b)b.disabled=false;});});}"
          "{const l=document.getElementById('nv-lbl');"
          "if(l)l.innerHTML='Viewing <b>'+hdrTxt+'</b>';}"
          "function fmtUp(sec){sec=Math.floor(sec);"
          "const d=(sec/86400)|0,h=((sec%86400)/3600)|0,"
          "m=((sec%3600)/60)|0;"
          "if(d)return d+'d '+h+'h';"
          "if(h)return h+'h '+m+'m';"
          "return m+'m';}"
          // initial populate of info card + footer
          "document.getElementById('i-host').textContent=location.hostname;"
          "document.getElementById('i-ip').textContent=location.hostname;"
          "document.getElementById('i-evt').textContent=EVENTS.length;"
          "document.getElementById('i-wx').textContent=wxl||'(none)';"
          "document.getElementById('i-ls').textContent=LAST_SYNC;"
          "document.getElementById('i-ss').textContent=STATUS;"
          // people labels + state
          "document.getElementById('i-pa-k').textContent=PA_LBL;"
          "document.getElementById('i-pb-k').textContent=PB_LBL;"
          "document.getElementById('i-pc-k').textContent=PC_LBL;"
          "document.getElementById('i-pd-k').textContent=PD_LBL;"
          "document.getElementById('i-pa').textContent=fmtPerson('',PA_ST,PA_SP).replace(/^: /,'');"
          "document.getElementById('i-pb').textContent=fmtPerson('',PB_ST,PB_SP).replace(/^: /,'');"
          "document.getElementById('i-pc').textContent=fmtPerson('',PC_ST,PC_SP).replace(/^: /,'');"
          "document.getElementById('i-pd').textContent=fmtPerson('',PD_ST,PD_SP).replace(/^: /,'');"
          "document.getElementById('i-fw').textContent=SW_VER||'(unset)';"
          "function getSensor(slug){return fetch('/sensor/'+slug)"
          ".then(r=>r.ok?r.json():null).catch(()=>null);}"
          "Promise.all([getSensor('wifi_rssi'),getSensor('device_uptime')])"
          ".then(([rssi,up])=>{"
          "let rTxt='?',uTxt='?';"
          "if(rssi&&rssi.value!=null){rTxt=Math.round(rssi.value)+' dBm';}"
          "if(up&&up.value!=null){uTxt=fmtUp(up.value);}"
          "document.getElementById('i-rssi').textContent=rTxt;"
          "document.getElementById('i-up').textContent=uTxt;"
          "const f=document.getElementById('fwifi');"
          "if(f)f.textContent='WiFi: '+rTxt+'  ·  IP: '+"
          "location.hostname+'  ·  Up: '+uTxt;});"
          "</script></body></html>";

      request->send(200, "text/html", html.c_str());
    }

  } // namespace preview_page
} // namespace esphome
