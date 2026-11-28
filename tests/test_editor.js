globalThis.runGooPadEditorTests = async function () {
 if (location.protocol !== "file:") throw Error("Run only against the local editor, never a device.");
 const original = structuredClone(cfg), confirmOriginal = globalThis.confirm, fetchOriginal = globalThis.fetch;
 const monitorOriginal = [pcMonitorPreview, pcStatsPreview, pcStatsFPreview];
 const siteThemeOriginal = document.querySelector("#siteTheme").value;
 const savedSiteThemeOriginal = localStorage.getItem("goopad-site-theme");
 let checks = 0, layouts = 0;
 const check = (value, message) => { checks++; if (!value) throw Error(message) };
 const make = i => ({...emptyMacro(), name:"W"+i, actionMode:"widget", widget:"clock"});
 const certificate="-----BEGIN CERTIFICATE-----\nAA==\n-----END CERTIFICATE-----";
 const reset = settings => {
  cfg = {...original, cols:3, rows:2, pageCount:3, hideUnconfigured:false, hideEmptyTiles:false, companionAddress:"",companionToken:"",companionCertificate:undefined, macros:Array.from({length:18}, (_,i)=>make(i)), ...settings};
  selectedTile = -1; setForm(); scrollPreviewPage(0);
 };
 const change = (key, value) => {
  const el = document.querySelector(`[data-k="${key}"]`);
  el.value = value; el.dispatchEvent(new Event("change"));
 };
 try {
  let versionStatus={ssid:"test",firmwareVersion:"12.34.56"};
  globalThis.fetch=async url=>{
   if(url==="/api/config")return new Response(JSON.stringify({...original,macros:[]}),{status:200});
   if(url==="/api/status")return new Response(JSON.stringify(versionStatus),{status:200});
   if(url==="/api/time")return new Response("{}",{status:200});
   throw Error("Unexpected request "+url);
  };
  await init();
  check(document.querySelector("#firmwareVersion").textContent==="Installed firmware: 12.34.56","About shows the running firmware version from status");
  versionStatus={ssid:"test"};
  await init();
  check(document.querySelector("#firmwareVersion").textContent==="Installed firmware: unavailable","About reports an unavailable version instead of a stale value");
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);
  const choice=(key,value)=>document.querySelector(`select[data-choice-enhanced="${key}"]`).nextElementSibling.querySelector(`input[value="${value}"]`);
  check(document.querySelector("#cols").hidden&&document.querySelector("#screenLayout").hidden&&document.querySelector("#siteTheme").hidden,"Layout and website theme use native choice groups");
  choice("siteTheme","ocean").click();
  check(document.body.dataset.siteTheme==="ocean"&&localStorage.getItem("goopad-site-theme")==="ocean","Website theme choice retains browser persistence");
  choice("cols","2").click();
  check(cfg.cols===2&&choice("cols","2").checked,"Column choice updates the layout draft");
  choice("cols","3").click();
  check(document.querySelector('[data-k="actionMode"]').hidden&&!choice("actionMode","advanced").checked,"Empty tiles present unselected action cards rather than a dropdown");
  choice("actionMode","advanced").click();
  check(cfg.macros[0].actionMode==="advanced"&&cfg.macros[0].action==="http"&&choice("actionMode","advanced").checked,"Action card initializes the existing request defaults");
  choice("action","webhook").click();choice("method","GET").click();
  check(cfg.macros[0].action==="webhook"&&cfg.macros[0].method==="GET","Request choices preserve the underlying saved values");
  choice("actionMode","smart_home").focus();choice("actionMode","smart_home").click();
  check(cfg.macros[0].actionMode==="home_assistant"&&document.activeElement===choice("actionMode","smart_home"),"Focus survives the Smart Home editor rebuild");
  choice("homeOperation","turn_off").click();
  check(cfg.macros[0].homeOperation==="turn_off","Home Assistant action choice updates the existing value");
  choice("actionMode","widget").click();choice("widgetCategory","news").click();
  check(cfg.macros[0].widget==="news"&&cfg.macros[0].newsFeed==="bbc_world"&&choice("widget","news").checked,"Widget category choices update the dependent widget and its defaults");
  choice("newsCount","5").click();
  check(cfg.macros[0].newsCount===5&&choice("newsCount","5").checked,"Headline count choice is saved as a number");
  document.querySelector('[data-news-extra="npr"]').click();
  document.querySelector('[data-news-extra="guardian"]').click();
  document.querySelector('[data-news-extra="verge"]').click();
  check(cfg.macros[0].newsExtras.join(",")==="npr,guardian,verge"&&document.querySelector('[data-news-extra="ars"]').disabled,"Additional news sources are bounded and saved");
  render();check(document.querySelector('[data-news-extra="npr"]').checked&&document.querySelector('[data-news-extra="bbc_world"]').disabled,"News source choices survive rerendering and exclude the primary");
  change("newsFeed","npr");
  check(cfg.macros[0].newsExtras.join(",")==="guardian,verge"&&document.querySelector('[data-news-extra="npr"]').disabled,"Changing the primary removes its duplicate from additional sources");
  prepareMacroAction(cfg.macros[0],0);
  let badSources=false;try{prepareMacroAction({...cfg.macros[0],newsExtras:["guardian","guardian"]},0)}catch{badSources=true}
  check(badSources,"Duplicate news sources are rejected");
  choice("widgetCategory","market").click();choice("widget","stock").click();choice("stockProvider","alphavantage").click();
  check(cfg.macros[0].stockProvider==="alphavantage"&&!document.querySelector('[data-stock-provider="alphavantage"]').classList.contains("hidden"),"Stock provider choice reveals its matching settings");
  choice("widgetCategory","clock").click();choice("widget","datetime").click();
  check(cfg.macros[0].widget==="datetime"&&!document.querySelector('[data-widget-option="datetime"]').classList.contains("hidden"),"Widget choice reveals its matching settings");
  document.querySelector('[data-section="icon"]').open=true;choice("widgetGraphic","custom_icon").click();choice("iconSource","gif").click();
  check(cfg.macros[0].widgetGraphic==="custom_icon"&&cfg.macros[0].iconSource==="gif"&&!document.querySelector('[data-visual="gif"]').classList.contains("hidden"),"Visual cards update conditional upload controls");
  document.querySelector('[data-section="appearance"]').open=true;choice("shape","circle").click();choice("widgetLayout","layered").click();
  const sizeSlider=document.querySelector('[data-k="textSize"] + .size-slider input');sizeSlider.value="3";sizeSlider.dispatchEvent(new Event("input"));
  check(cfg.macros[0].textSize===20&&sizeSlider.getAttribute("aria-valuetext")==="20","Accessible label-size slider retains the existing numeric configuration");
  render();check(choice("shape","circle").checked&&choice("widgetLayout","layered").checked&&choice("iconSource","gif").checked&&cfg.macros[0].shape==="circle","Appearance and visual choices survive rerendering");
  cfg.macros[0]={...make(0),widget:"github"};render();
  check(choice("widgetCategory","legacy").checked&&choice("widgetCategory","legacy").disabled&&choice("widget","github").checked,"Existing legacy widget remains visible without introducing a new legacy choice");
  reset({pageCount:1,macros:[{...emptyMacro(),actionMode:"page_navigation"}]});selectTile(0);
  check(choice("actionMode","page_navigation").checked,"Existing navigation actions remain selectable and visible");
  check(!document.querySelector('[data-k="widgetAccent"]').closest(".field").classList.contains("hidden"),"Action tiles expose the card accent control");
  setDeviceDisplay({screenWidth:240,screenHeight:240,maxCols:3,maxRows:2});
  check([...document.querySelector('#rows').options].find(option=>option.value==="4").disabled,"Small screens disable excessively dense grids");
  check(choice("rows","4").disabled&&choice("cols","4").disabled,"Unsupported grid choices are visible but disabled");
  const small=widgetDimensions({shape:"rounded"},3,2);
  check(small.width===68&&small.height===64,"Preview geometry follows the attached display rather than 480x480");
  check(document.querySelector("#preview").style.aspectRatio==="240 / 156","Preview aspect ratio follows the available viewport");
  let invalidDisplay=false;try{setDeviceDisplay({screenWidth:240})}catch{invalidDisplay=true}
  check(invalidDisplay,"Partial display capabilities are rejected rather than silently replaced");
  setDeviceDisplay({});
  check(![...document.querySelector('#rows').options].find(option=>option.value==="4").disabled,"Legacy firmware retains 480x480 grid support");
  check(!choice("rows","4").disabled&&!choice("cols","4").disabled,"Grid choices re-enable when display capabilities change");
  reset({cols:3,rows:2,pageCount:1,macros:[{...make(0),spanCols:2},emptyMacro(),make(2)]});
  const pageBounds=document.querySelector(".preview-page").getBoundingClientRect();
  check(pageBounds.width>0&&pageBounds.height>0,"Grid preview is laid out before checking geometry");
  for(const [index,x,y,width,height] of [[0,10,10,304,184],[2,322,10,148,184],[3,10,202,148,184]]){
   const cell=document.querySelector(`[data-tile="${index}"]`).parentElement.getBoundingClientRect();
   for(const [actual,expected] of [[cell.left-pageBounds.left,x*pageBounds.width/480],[cell.top-pageBounds.top,y*pageBounds.height/396],[cell.width,width*pageBounds.width/480],[cell.height,height*pageBounds.height/396]])
    check(Math.abs(actual-expected)<.1,"Grid preview uses the device's scaled padding, gap and integer tile geometry");
  }
  check(getComputedStyle(document.querySelector('[data-tile="3"]')).backgroundColor==="rgba(27, 36, 50, 0.7)","Empty tile background uses the device's 70% opacity");
  reset({cols:3,rows:3,pageCount:1,macros:[make(0)]});selectTile(0);
  check(!document.querySelector("#showBattery").checked&&document.querySelector("#previewBattery").classList.contains("hidden"),"Battery indicator defaults off");
  document.querySelector("#showBattery").checked=true;document.querySelector("#showBattery").dispatchEvent(new Event("change"));
  batteryPreview=null;updateWidgetPreview();check(document.querySelector("#previewBattery").textContent==="Battery unavailable","Unsupported telemetry never invents a percentage");
  batteryPreview=65;updateWidgetPreview();check(document.querySelector("#previewBattery").textContent==="Battery 65%","Available battery percentage is displayed");
  check(document.querySelector('#screenLayout option[value="ring"]').textContent==="Circles"&&document.querySelector('#screenLayout option[value="arches"]')&&!document.querySelector('#screenLayout option[value="arc"]'),"Circles and Arches are both available; legacy arc is retained only when loaded");
  for(const layout of ["arches","ring","arc"]){
   if(layout!=="arches"){cfg.screenLayout=layout;render()}
   const control=document.querySelector("#screenLayout");control.value=layout;control.dispatchEvent(new Event("change"));
   check(cfg.screenLayout===layout&&document.querySelector(".preview-page.radial")&&document.querySelector('[data-k="spanCols"]').disabled,"Ring layout is previewed and disables enlargement: "+layout);
   check(choice("screenLayout",layout).checked&&choice("spanCols","2").disabled,"Layout and disabled width choices track radial layout: "+layout);
   const round=widgetDimensions({shape:"rounded"},3,3),pill=widgetDimensions({shape:"pill"},3,3);
   check(pill.width===round.width&&pill.height===Math.floor(round.height*3/4),"Radial pill content uses the device's reduced tile height: "+layout);
   const tiles=[...document.querySelectorAll(".preview-cell")].map(cell=>cell.getBoundingClientRect());
   for(let i=0;i<tiles.length;i++)for(let j=0;j<i;j++)check(tiles[i].right<=tiles[j].left+.1||tiles[j].right<=tiles[i].left+.1||tiles[i].bottom<=tiles[j].top+.1||tiles[j].bottom<=tiles[i].top+.1,"Ring tiles do not overlap");
  }
  document.querySelector("#screenLayout").value="grid";document.querySelector("#screenLayout").dispatchEvent(new Event("change"));
  change("spanCols","2");document.querySelector("#screenLayout").value="arches";document.querySelector("#screenLayout").dispatchEvent(new Event("change"));
  check(cfg.screenLayout==="grid"&&document.querySelector("#screenLayout").value==="grid","Ring selection refuses existing enlarged tiles without changing them");
  choice("screenLayout","arches").click();
  check(cfg.screenLayout==="grid"&&choice("screenLayout","grid").checked,"Rejected layout choice restores the selected radio");
  choice("spanCols","4").click();
  check(cfg.macros[0].spanCols===2&&choice("spanCols","2").checked,"Rejected tile width restores its selected radio");
  change("spanCols","1");
  change("widgetTheme","custom");
  const gradientRoot=()=>document.querySelector("[data-gradient-editor]");
  gradientRoot().querySelector("[data-gradient-add]").click();
  check(cfg.macros[0].gradientStops.length===3&&cfg.macros[0].gradientStops[1].position===50,"Gradient editor inserts an interior color stop");
  const slider=gradientRoot().querySelector("[data-gradient-position]");slider.value="35";slider.dispatchEvent(new Event("input"));
  check(slider===gradientRoot().querySelector("[data-gradient-position]")&&cfg.macros[0].gradientStops[1].position===35&&gradientRoot().querySelector("output").textContent==="35%","Gradient slider updates without replacing itself during a drag");
  const stopColor=gradientRoot().querySelector("[data-gradient-color]");stopColor.value="#00ff00";stopColor.dispatchEvent(new Event("input"));
  for(const direction of ["horizontal","vertical","circle"]){
   choice("gradientDirection",direction).click();prepareMacroAction(cfg.macros[0],0);
   const background=document.querySelector('[data-tile="0"]').style.backgroundImage;
   check(background.includes(direction==="circle"?"radial-gradient":"linear-gradient")&&cfg.macros[0].gradientDirection===direction,"Directional multi-stop gradient is rendered and persisted: "+direction)
  }
  for(let i=0;i<3;i++)gradientRoot().querySelector("[data-gradient-add]").click();
  check(cfg.macros[0].gradientStops.length===6&&gradientRoot().querySelector("[data-gradient-add]").disabled,"Gradient editor caps stop count at six");
  gradientRoot().querySelector("[data-gradient-remove]").click();
  check(cfg.macros[0].gradientStops.length===5&&validGradient(cfg.macros[0]),"Removing an interior gradient stop preserves ordered endpoints");
  for(const bad of [{gradientDirection:"diagonal"},{gradientStops:null},{gradientStops:[{color:"GGGGGG",position:0},{color:"000000",position:100}]},{gradientStops:[{color:"FFFFFF",position:0},{color:"000000",position:0}]}]){
   let rejected=false;try{prepareMacroAction({...cfg.macros[0],...bad},0)}catch{rejected=true}
   check(rejected,"Malformed gradient is rejected before saving")
  }
  for(const key of ["fontBold","fontItalic"]){const control=document.querySelector(`[data-k="${key}"]`);control.checked=true;control.dispatchEvent(new Event("change"))}
  choice("textDecoration","underline").click();choice("textSpacing","2").click();
  check(cfg.macros[0].textDecoration==="underline"&&cfg.macros[0].textSpacing===2,"Text choice groups preserve saved value types");
  check([...document.querySelectorAll('[data-tile="0"] small,[data-tile="0"] .widget')].every(text=>getComputedStyle(text).fontWeight==="700"&&getComputedStyle(text).fontStyle==="italic"),"Bold and italic apply to caption and widget readings");
  const emphasisCopy=structuredClone(cfg.macros[0]);prepareMacroAction(emphasisCopy,0);check(emphasisCopy.fontBold&&emphasisCopy.fontItalic,"Font emphasis survives serialization");
  for(const section of document.querySelectorAll(".tile-section"))section.open=true;
  check([...document.querySelectorAll(".macro select")].filter(control=>control.getBoundingClientRect().height).every(control=>control.getBoundingClientRect().height===42),"Select controls remain a single uniform height beside long hints");
  delete cfg.macros[0].gradientStops;delete cfg.macros[0].gradientDirection;delete cfg.macros[0].fontBold;delete cfg.macros[0].fontItalic;
  cfg.macros[0].widgetTheme="cloud";
  cfg.macros[1]={...make(1),widgetTheme:"custom"};
  cfg.macros[0].outline=true;
  for(const theme of ["liquid_glass","material"]){
   document.querySelector("#deviceTheme").value=theme;document.querySelector("#deviceTheme").dispatchEvent(new Event("change"));
   check(document.querySelector('[data-tile="0"]').classList.contains(theme==="liquid_glass"?"liquid-glass":"material"),"New device theme styles default tiles: "+theme);
   check(cfg.macros.every(m=>m.widgetTheme==="device")&&document.querySelector('[data-tile="1"]').classList.contains(theme==="liquid_glass"?"liquid-glass":"material"),"Device theme selection replaces old per-tile overrides consistently");
  }
  cfg.macros[0].widgetTheme="custom";render();document.querySelector("#applyDeviceTheme").click();
  check(cfg.macros[0].widgetTheme==="device"&&document.querySelector('[data-tile="0"]').classList.contains("material"),"Apply to every tile unifies the current saved palette without requiring a different theme");
  check(previewIconColor({widgetTheme:"device"})==="1D1B20"&&previewIconColor({widgetTheme:"device",iconColor:"ABCDEF"})==="ABCDEF","Font Awesome preview inherits preset text color but preserves explicit tint");
  const materialTile=document.querySelector('[data-tile="0"]');
  check(materialTile.style.backgroundColor==="rgb(243, 237, 247)"&&getComputedStyle(materialTile).backgroundImage==="none"&&!materialTile.querySelector(".modern-accent")&&materialTile.style.borderColor==="rgb(202, 196, 208)","Material uses flat tonal containers and subtle outlines without decorative strips");
  document.querySelector("#deviceTheme").value="liquid_glass";document.querySelector("#deviceTheme").dispatchEvent(new Event("change"));
  check(getComputedStyle(document.querySelector('[data-tile="0"]')).backdropFilter.includes("blur")&&document.querySelector("#preview").style.backgroundImage.includes("radial-gradient"),"Liquid glass blurs a soft colored backdrop");
  document.querySelector("#deviceTheme").value="material";document.querySelector("#deviceTheme").dispatchEvent(new Event("change"));
  check(document.querySelector("#preview").style.backgroundImage==="none","Switching themes clears the previous glass/image backdrop");
  for(const width of [240,360,480])for(const cols of [2,3,4])for(const rows of [2,3,4]){
   const height=width-84,radius=height/2-10,tiles=[];
   for(let slot=0;slot<cols*rows;slot++){
    const tile=archesPosition(width,height,cols,rows,slot);
    for(const x of [0,1])for(const y of [0,1])check((tile.x+x*tile.side-width/2)**2+(tile.y+y*tile.side-height/2)**2<=radius**2,"Arches square corners fit inside the circular viewport");
    for(const other of tiles)check(tile.x+tile.side+6<=other.x||other.x+other.side+6<=tile.x||tile.y+tile.side+6<=other.y||other.y+other.side+6<=tile.y,"Arches tiles retain separation");
    tiles.push(tile)
   }
   if(cols>=3)check(tiles[1].y<tiles[0].y,"Arches visibly bows each row upwards instead of distributing a single ring")
   check(Math.abs(tiles.reduce((sum,tile)=>sum+tile.y+tile.side/2,0)/tiles.length-height/2)<=1,"Arches stays centered rather than drifting upwards");
  }
  reset({pageCount:1,macros:[{...make(0),widgetLayout:"layered",widgetGraphic:"text"}]});
  const layeredWidget=document.querySelector('[data-tile="0"]'),surface=getComputedStyle(layeredWidget,"::after"),widgetBounds=layeredWidget.querySelector(".widget").getBoundingClientRect();
  const surfaceCenter=layeredWidget.getBoundingClientRect().top+layeredWidget.clientTop+parseFloat(surface.top)+parseFloat(surface.height)/2;
  check(Math.abs((widgetBounds.top+widgetBounds.bottom)/2-surfaceCenter)<2,"Layered widget text centers in its inset surface above the caption");
  selectTile(0);
  change("widget","news");
  check(cfg.macros[0].newsSource==="rss"&&cfg.macros[0].newsFeed==="bbc_world"&&!document.querySelector('[data-k="newsSource"]'),"New news widgets default to RSS without JSON source choices");
  for(const key of Object.keys(newsFeeds)){change("newsFeed",key);prepareMacroAction(cfg.macros[0],0);render();check(cfg.macros[0].newsFeed===key&&document.querySelector('[data-k="newsFeed"]').value===key&&!cfg.macros[0].newsUrl,"RSS preset survives saving and reopening: "+key)}
  change("newsFeed","custom");change("newsUrl","https://example.com/feed.xml");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].newsUrl==="https://example.com/feed.xml"&&!document.querySelector("[data-news-url]").classList.contains("hidden"),"Custom RSS enables its HTTPS URL");
  for(const bad of [{newsFeed:"unknown"},{newsFeed:"constructor"},{newsFeed:null},{newsFeed:"custom",newsUrl:"http://example.com/feed.xml"},{newsFeed:"custom",newsUrl:""}]){
   let rejected=false;try{prepareMacroAction({...cfg.macros[0],...bad},0)}catch{rejected=true}
   check(rejected,"RSS rejects invalid preset identities and missing/unverified custom URLs")
  }
  cfg.macros[0].newsSource="jsonfeed";cfg.macros[0].newsUrl="https://example.com/feed.json";render();
  prepareMacroAction(cfg.macros[0],0);render();
  check(cfg.macros[0].newsUrl==="https://example.com/feed.json"&&document.querySelector('[data-k="newsFeed"]').value===""&&!document.querySelector("[data-news-legacy]").classList.contains("hidden"),"Untouched legacy source is preserved without pretending an RSS feed is selected");
  let invalidFeed=false;try{prepareMacroAction({...cfg.macros[0],newsUrl:"http://example.com/feed.json"},0)}catch{invalidFeed=true}
  check(invalidFeed,"News feeds require verified HTTPS");
  liveWidgetPreview=["First headline\nSecond headline"];updateWidgetPreview();
  check(document.querySelector('[data-tile="0"] .widget').textContent==="First headline\n---\nSecond headline","News tiles separate cached headlines visibly");
  change("newsFeed","hackernews");change("newsCount","5");prepareMacroAction(cfg.macros[0],0);render();
  check(cfg.macros[0].newsSource==="rss"&&!cfg.macros[0].newsUrl&&cfg.macros[0].newsCount===5&&document.querySelector('[data-k="newsCount"]').value==="5","Selecting RSS explicitly migrates legacy news and persists the headline count");
  change("newsCount","1");check(document.querySelector('[data-tile="0"] .widget').textContent==="First headline","Draft headline count limits the cached preview immediately");change("newsCount","5");
  for(const newsCount of [0,6,1.5,"3",null]){
   let rejected=false;try{prepareMacroAction({...cfg.macros[0],newsCount},0)}catch{rejected=true}
   check(rejected,"Headline count rejects values outside the supported integer range")
  }
  requestPreview=[];batteryPreview=null;liveWidgetPreview=[];
  const sampleTime=new Date("2026-11-07T14:03:09Z");
  check(percentTimeText("%A %B %-d%n%-I:%M:%S %p %%",sampleTime)==="Saturday November 7\n2:03:09 PM %","Percent formats support names, unpadded values, newlines and literals");
  check(widgetTimeText({widget:"clock",clockFormat:"12h_seconds"},sampleTime)==="2:03:09 PM","Legacy clock formats retain their readings");
  check(widgetTimeText({widget:"date",dateFormat:"mdy"},sampleTime)==="11/07/2026","Legacy date formats retain their readings");
  for(const format of ["%","%Q","%-A","%100Y","%z","x".repeat(97),"bad\rformat"]){
   let rejected=false;try{prepareMacroAction({...make(0),clockFormat:format},0)}catch{rejected=true}
   check(rejected,"Invalid percent format is rejected: "+format);
  }
  reset({pageCount:1,macros:[make(0)]});selectTile(0);change("widget","datetime");
  check(!document.querySelector('[data-widget-option="datetime"]').classList.contains("hidden"),"Date & time widget exposes its format");
  change("datetimeFormat","%a %b %-d%n%H:%M");prepareMacroAction(cfg.macros[0],0);render();
  check(document.querySelector('[data-k="datetimeFormat"]').value==="%a %b %-d%n%H:%M"&&widgetTimeText(cfg.macros[0],sampleTime)==="Sat Nov 7\n14:03","Combined format survives preparation and reopening");
  for(const mode of ["advanced","sound","pc_macro","homekit","wake_on_lan","display"]){
   reset({pageCount:1,macros:[{...emptyMacro(),actionMode:mode,url:"https://example.com/",icon:"★",sound:"beep",iconSource:"emoji"}]});selectTile(0);
   check(!document.querySelector('[data-k="widgetTheme"]').closest(".field").classList.contains("hidden"),"Preset themes are available on "+mode);
   change("widgetTheme","paper");
   check(document.querySelector('[data-tile="0"]').style.color==="rgb(23, 32, 51)"&&document.querySelector('[data-k="color"]').disabled,"Action theme applies text and locks custom colors");
   change("widgetTheme","custom");check(!document.querySelector('[data-k="color"]').disabled,"Custom colors can be restored");
  }
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","advanced");change("url","https://example.com/");
  check(cfg.macros[0].action==="http"&&document.querySelector('[data-tile="0"] [data-widget="response"]'),"New web request defaults to displaying its response");
  requestPreview=["<b>42</b>"];updateWidgetPreview();
  check(document.querySelector('[data-tile="0"] .widget').textContent==="<b>42</b>"&&!document.querySelector('[data-tile="0"] .widget b'),"HTTP response renders text, not HTML");
  const statusFetch=globalThis.fetch;
  globalThis.fetch=async url=>url==="/api/status"?new Response(JSON.stringify({pcMonitor:"PC unavailable",pcStats:Object.fromEntries(Object.keys(pcStatLabels).map(key=>[key,"Unavailable"])),pcStatsF:{cpu_temp:"Unavailable",gpu_temp:"Unavailable"},requestResults:["temperature: 42"]})):statusFetch(url);
  await refreshMonitorPreview();
  check(document.querySelector('[data-tile="0"] .widget').textContent==="temperature: 42","A GET result from device status reaches its matching preview tile even when PC monitoring is unavailable");
  globalThis.fetch=statusFetch;
  change("action","webhook");check(!document.querySelector('[data-tile="0"] .widget'),"Send-only webhook does not display response data");
  const fileKey="i0000000000000001",backgroundKey="i0000000000000002",unusedKey="i0000000000000003";
  reset({pageCount:1,backgroundImage:backgroundKey,macros:[{...make(0),iconImage:fileKey}]});
  let deletedFile,failDelete=false;
  globalThis.confirm=()=>true;
  globalThis.fetch=async(url,options={})=>{
   if(url==="/api/saved-icons?offset=0&backgrounds=1")return new Response(JSON.stringify({assets:[{key:fileKey,format:"rgb565a8",size:160},{key:backgroundKey,format:"rgb565",size:480},{key:unusedKey,format:"gif",size:160}],next:24}));
   if(url===`/api/icon?key=${fileKey}&format=rgb565a8&size=160`)return new Response(new Uint8Array(160*160*3));
   if(url===`/api/icon?key=${backgroundKey}&format=rgb565&size=480`)return new Response(new Uint8Array(480*480*2));
   if(url===`/api/icon?key=${unusedKey}&format=gif&size=160`)return new Response(Uint8Array.from(atob("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"),ch=>ch.charCodeAt(0)));
   if(options.method==="DELETE"){deletedFile=url;return new Response(failDelete?"Icon is still used by the layout":"{}",{status:failDelete?409:200})}
   throw Error("Unexpected file manager request "+url);
  };
  await showImageFiles();
  check([...document.querySelectorAll("#filesList img")].length===3&&document.querySelector("#filesList img").src.startsWith("data:image/png"),"About displays decoded image, background and GIF thumbnails");
  check([...document.querySelectorAll("#filesList img")][1].src.startsWith("data:image/png")&&[...document.querySelectorAll("#filesList img")][2].src.startsWith("data:image/gif"),"Background pixels and original animated GIF use the shared thumbnail loader");
  const deleteButtons=()=>[...document.querySelectorAll("#filesList button")];
  check(deleteButtons()[0].disabled&&deleteButtons()[1].disabled&&!deleteButtons()[2].disabled,"File manager protects draft images and backgrounds");
  check(!document.querySelector("#filesNext").classList.contains("hidden"),"File manager exposes pagination");
  await deleteButtons()[2].onclick();check(deletedFile==="/api/icon?key="+unusedKey,"File manager uses protected deletion API");
  failDelete=true;await deleteButtons()[2].onclick();
  check(document.querySelector("#filesStatus").textContent.includes("Icon is still used")&&!deleteButtons()[2].disabled,"Deletion failures stay visible and can be retried");
  globalThis.fetch=fetchOriginal;globalThis.confirm=confirmOriginal;
  globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);
  const actionOptions=[...document.querySelector('[data-k="actionMode"]').options].map(option=>option.textContent);
  check(actionOptions.includes("Widgets")&&actionOptions.includes("Macro")&&actionOptions.includes("Smart Home")&&actionOptions.includes("Image Only"),"Action options use title case without changing their values");
  check(document.querySelector(".layout-settings summary").textContent==="Layout Settings"&&document.querySelector('[data-k="widgetTheme"]').closest(".field").querySelector("label").textContent==="Tile Theme"&&document.querySelector("#save").textContent==="Save to Device","Layout and tile labels use consistent capitalization");
  cfg.macros[0].name="my Custom tile";render();check(document.querySelector('[data-k="name"]').value==="my Custom tile"&&cfg.macros[0].name==="my Custom tile","User-entered tile names retain their original capitalization");
  check(!document.querySelector('[data-k="actionMode"] option[value="wake_on_lan"]'),"Wake-on-LAN is not a top-level action");
  check(document.querySelector("#macrosTab").textContent==="Tiles"&&document.querySelector("#aboutTab").textContent==="About","Navigation uses Tiles and About");
  check([...document.querySelectorAll("#aboutView details")].every(details=>!details.open),"Help FAQs start collapsed");
  change("actionMode","widget");
  const chooseGroup=group=>{const control=document.querySelector("[data-widget-category]");control.value=group;control.dispatchEvent(new Event("change"))};
  for(const [group,values] of Object.entries(widgetGroups)){
   chooseGroup(group);
   const selector=document.querySelector('[data-k="widget"]');
   check(selector.value===values[0]&&[...selector.options].filter(option=>!option.hidden).map(option=>option.value).join(",")===values.join(","),"Widget category exposes its suboptions: "+group);
   for(const value of values){change("widget",value);render();check(document.querySelector("[data-widget-category]").value===group&&document.querySelector('[data-k="widget"]').value===value,"Widget selection reopens in its category: "+value)}
  }
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","widget");change("widget","alarm");
  check(!document.querySelector('[data-widget-option="alarm"]').classList.contains("hidden"),"Alarm settings appear under Clock");
  change("alarmTime","06:45");change("alarmSound","beep");
  const enabled=document.querySelector('[data-k="alarmEnabled"]');enabled.checked=false;enabled.dispatchEvent(new Event("change"));
  let savedAlarm;
  globalThis.fetch=async (url,options={})=>url==="/api/config"&&options.method==="POST"
   ? (savedAlarm=JSON.parse(options.body),new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  globalThis.fetch=fetchOriginal;
  check(savedAlarm?.macros[0].alarmTime==="06:45"&&savedAlarm.macros[0].alarmEnabled===false&&savedAlarm.macros[0].alarmSound==="beep"&&/^a[0-9a-f]{14}$/.test(savedAlarm.macros[0].alarmId),"Alarm save includes settings and a stable identity");
  cfg.macros[0]=savedAlarm.macros[0];render();
  check(document.querySelector('[data-k="alarmTime"]').value==="06:45"&&!document.querySelector('[data-k="alarmEnabled"]').checked&&document.querySelector('[data-k="alarmSound"]').value==="beep","Alarm settings reopen intact");
  const id=cfg.macros[0].alarmId;change("alarmTime","08:30");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].alarmId===id,"Editing an alarm preserves its identity");
  for(const bad of [{alarmTime:"24:00"},{alarmEnabled:"true"},{alarmSound:"none"},{alarmId:"invalid"}]){
   let rejected=false;try{prepareMacroAction({...cfg.macros[0],...bad},0)}catch{rejected=true}
   check(rejected,"Invalid alarm settings are rejected");
  }
  change("widget","clock");
  check(document.querySelector('[data-widget-option="alarm"]').classList.contains("hidden"),"Switching away hides alarm settings");
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","widget");
  check(!document.querySelector('[data-k="widget"] option[value="github"]'),"New widgets do not offer GitHub profile stats");
  change("actionMode","smart_home");
  for(const type of ["home_assistant","homekit","home_state"]){
   const control=document.querySelector("[data-smart-home-type]");control.value=type;control.dispatchEvent(new Event("change"));
   check(document.querySelector('[data-k="actionMode"]').value==="smart_home"&&document.querySelector("[data-smart-home-type]").value===type,"Smart home selection survives rerender: "+type);
  }
  change("actionMode","widget");
  check(cfg.macros[0].widget==="clock"&&!document.querySelector('[data-mode-field="widget"]').classList.contains("hidden"),"Switching from Home Assistant state to Widgets exposes Clock");
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","display");
  let missingImage=false;try{prepareMacroAction(structuredClone(cfg.macros[0]),0)}catch{missingImage=true}
  check(missingImage&&document.querySelector('[data-section="sound"]').classList.contains("hidden"),"Image-only tiles require an image and hide sound controls");
  change("iconSource","image");Object.assign(cfg.macros[0],{iconImage:"i0000000000000001",iconFormat:"rgb565a8",iconSize:160});
  const display=structuredClone(cfg.macros[0]);prepareMacroAction(display,0);
  check(display.actionMode==="simple_link"&&display.url===""&&display.sound==="none"&&display.iconImage==="i0000000000000001","Image-only tiles save in the compatible display-only format");
  const formerWeather={...cfg.macros[0],widget:"weather",widgetGraphic:"text"};prepareMacroAction(formerWeather,0);
  check(!("widget" in formerWeather)&&!("widgetGraphic" in formerWeather),"Image-only conversion does not retain an incomplete widget type");
  let savedDisplay;
  globalThis.fetch=async (url,options={})=>url==="/api/config"&&options.method==="POST"
   ? (savedDisplay=JSON.parse(options.body),new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(savedDisplay?.macros[0].actionMode==="simple_link"&&savedDisplay.macros[0].iconImage===display.iconImage,"Image-only tile is submitted as a display-only record");
  globalThis.fetch=fetchOriginal;cfg.macros[0]=savedDisplay.macros[0];render();
  check(document.querySelector('[data-k="actionMode"]').value==="display","Saved image-only tiles reopen as Image only");
  change("visualFit","cover");
  check(document.querySelector('[data-k="widgetLayout"]').closest(".field").classList.contains("hidden"),"Covered visuals hide inapplicable card styles");
  change("visualFit","contain");
  check(!document.querySelector('[data-k="widgetLayout"]').closest(".field").classList.contains("hidden"),"Contained visuals expose card styles");
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","sound");
  let missingSound=false;try{prepareMacroAction(structuredClone(cfg.macros[0]),0)}catch{missingSound=true}
  check(missingSound&&document.querySelector('[data-mode-field="sound"]').textContent.includes("required")&&document.querySelector('[data-section="sound"] .section-hint').textContent==="Required sound","Sound-only guidance matches save validation");
  reset({pageCount:1,macros:[make(0)]});selectTile(0);
  const clockOptions=document.querySelector('[data-widget-option="clock"]');
  check(!clockOptions.classList.contains("hidden"),"Clock format options are visible when editing a clock");
  check(!document.querySelector('[data-k="widgetGraphic"] option[value="analog"]').hidden,"Clock exposes its analog graphic");
  change("clockFormat","12h_seconds");change("widgetGraphic","analog");change("widgetGraphic","text");
  check(!clockOptions.classList.contains("hidden")&&cfg.macros[0].clockFormat==="12h_seconds","Clock options stay visible and preserve the selected format after other widget edits");
  render();
  check(!document.querySelector('[data-widget-option="clock"]').classList.contains("hidden")&&document.querySelector('[data-k="clockFormat"]').value==="%-I:%M:%S %p","Legacy clock format reopens as an editable percent format");
  change("widget","exchange_rate");
  check(!document.querySelector('[data-widget-option="exchange_rate"]').classList.contains("hidden"),"Exchange rate currency fields are visible");
  change("currencyBase","usd");change("currencyQuote","eur");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].currencyBase==="USD"&&cfg.macros[0].currencyQuote==="EUR","Exchange currencies are normalized on save");
  change("currencyQuote","USD");
  let currencyRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{currencyRejected=true}
  check(currencyRejected,"Exchange rate requires different currencies");
  change("widget","weather_alerts");
  check(!document.querySelector('[data-widget-option="weather"]').classList.contains("hidden")&&!document.querySelector('[data-widget-option="weather_alerts"]').classList.contains("hidden"),"NWS alerts expose coordinates and safety warning");
  let locationRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{locationRejected=true}
  check(locationRejected,"NWS alerts require coordinates");
  change("weatherLatitude","40");change("weatherLongitude","-74");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].weatherLatitude===40&&cfg.macros[0].weatherLongitude===-74,"NWS alert coordinates are stored as numbers");
  change("widget","home_state");
  const homeFields=document.querySelector('[data-widget-option="home_state"]');
  check(!homeFields.classList.contains("hidden"),"Home Assistant state settings are visible");
  for(const [key,value] of [["homeAddress","https://home.example.com"],["homeEntity","sensor.temperature"],["homeToken","test-token"]]){
   const input=homeFields.querySelector(`[data-k="${key}"]`);input.value=value;input.dispatchEvent(new Event("change"));
  }
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].url==="https://home.example.com/api/states/sensor.temperature","Home Assistant state uses the entity endpoint");
  cfg.macros[0].homeAddress="http://home.example.com";
  let insecureHomeRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{insecureHomeRejected=true}
  check(insecureHomeRejected,"Home Assistant state rejects insecure connections");
  cfg.companionAddress="https://192.168.1.5:8787";cfg.companionToken="t".repeat(40);cfg.companionCertificate=certificate;
  change("widget","now_playing");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].url==="https://192.168.1.5:8787/monitor","Now playing uses the secure paired Companion");
  check(document.querySelector('[data-k="widgetGraphic"] option[value="analog"]').hidden&&!document.querySelector('[data-k="widgetGraphic"] option[value="custom_icon"]').hidden,"Now playing hides incompatible graphics but retains custom icons");
  change("widget","pc_monitor");change("pcStat","cpu");
  check(!document.querySelector('[data-k="widgetGraphic"] option[value="ring"]').hidden,"Percentage PC stats expose the usage ring");
  change("pcStat","cpu_temp");
  check(document.querySelector('[data-k="widgetGraphic"] option[value="ring"]').hidden,"Temperature stats hide percentage graphics");
  change("widget","clock");
  check(document.querySelector('[data-k="widgetLayout"]').value==="panel"&&!!document.querySelector('[data-tile="0"] .widget-heading'),"Unspecified widget layouts use modern cards");
  change("widgetLayout","classic");
  check(!document.querySelector('[data-tile="0"] .widget-heading')&&!document.querySelector('[data-tile="0"]').classList.contains("caption-rail"),"Classic widget has no modern heading or layered surfaces");
  change("widgetLayout","layered");
  check(!document.querySelector('[data-tile="0"] .widget-heading')&&document.querySelector('[data-tile="0"]').classList.contains("caption-rail")&&document.querySelector('[data-tile="0"]').classList.contains("widget-surface"),"Layered widget centers its content without a modern heading");
  render();
  check(document.querySelector('[data-k="widgetLayout"]').value==="layered"&&JSON.parse(JSON.stringify(cfg)).macros[0].widgetLayout==="layered","Widget style survives rerender and serialization");
  reset({cols:3,rows:3,pageCount:1,macros:[{...emptyMacro(),name:"Lights",icon:"★",url:"https://example.com/"}]});selectTile(0);
  let action=document.querySelector('[data-tile="0"]');
  check(!document.querySelector('[data-k="widgetLayout"]').closest(".field").classList.contains("hidden")&&document.querySelector('[data-k="widgetLayout"]').value==="panel","Actions expose the card style selector");
  check(action.classList.contains("action-card")&&!action.classList.contains("action-medallion")&&action.querySelector("small").style.textAlign==="left"&&document.querySelector('[data-k="textAlign"]').value==="left","Default action tiles retain the modern layout without layered surfaces");
  change("widgetLayout","classic");
  check(!document.querySelector('[data-tile="0"]').classList.contains("action-card")&&!document.querySelector('[data-tile="0"]').classList.contains("caption-rail")&&document.querySelector('[data-k="textAlign"]').value==="center","Classic action uses the centered layout");
  change("widgetLayout","layered");
  action=document.querySelector('[data-tile="0"]');
  check(!action.classList.contains("action-card")&&action.classList.contains("action-medallion")&&action.classList.contains("caption-rail"),"Layered action uses centered surfaces, not Modern's left-aligned layout");
  prepareMacroAction(cfg.macros[0],0);render();
  check(document.querySelector('[data-k="widgetLayout"]').value==="layered"&&JSON.parse(JSON.stringify(cfg)).macros[0].widgetLayout==="layered","Action style survives preparation and reload");
  action=document.querySelector('[data-tile="0"]');
  const iconBounds=action.querySelector(".ico").getBoundingClientRect(),captionBounds=action.querySelector("small").getBoundingClientRect();
  check(iconBounds.bottom<=captionBounds.top,"Action icon and caption do not overlap");
  const tileBounds=action.getBoundingClientRect(),plateStyle=getComputedStyle(action,"::after"),plateLeft=parseFloat(plateStyle.left),plateWidth=parseFloat(plateStyle.width),plateShift=new DOMMatrix(plateStyle.transform).m41;
  check(Math.abs((iconBounds.left+iconBounds.right)/2-(tileBounds.left+tileBounds.right)/2)<1,"Layered visual is horizontally centered");
  check(Math.abs((iconBounds.left+iconBounds.right)/2-(tileBounds.left+action.clientLeft+plateLeft+plateWidth/2+plateShift))<1,"Layered visual and medallion share a center");
  change("textSize","40");
  action=document.querySelector('[data-tile="0"]');
  check(action.querySelector(".ico").getBoundingClientRect().bottom<=action.querySelector("small").getBoundingClientRect().top,"Large captions leave room for the action icon");
  change("textAlign","right");
  check(document.querySelector('[data-tile="0"] small').style.textAlign==="right","Explicit caption alignment is preserved");
  change("shape","circle");
  check(!document.querySelector('[data-tile="0"]').classList.contains("action-card"),"Curved action tiles keep their centered layout");
  change("widgetLayout","panel");
  check(!!document.querySelector('[data-tile="0"] .modern-accent'),"Modern remains visibly distinct on curved tiles");
  change("widgetLayout","layered");
  check(document.querySelector('[data-tile="0"]').classList.contains("caption-rail")&&document.querySelector('[data-tile="0"]').classList.contains("action-medallion"),"Curved action tiles receive the new icon and caption surfaces");
  reset({cols:3,rows:3,pageCount:1,macros:[{...make(0),shape:"soft",widgetLayout:"layered"}]});
  check(document.querySelector('[data-tile="0"]').classList.contains("caption-rail")&&document.querySelector('[data-tile="0"]').classList.contains("widget-surface")&&!document.querySelector('[data-tile="0"] .widget-heading'),"Layered curved widgets have inset surfaces without a clipped heading");
  reset({cols:3,rows:3,pageCount:1,macros:[{...emptyMacro(),name:"Animation",url:"https://example.com/",iconFormat:"gif",gifFit:"cover",widgetLayout:"layered"}]});
  check(!document.querySelector('[data-tile="0"]').classList.contains("caption-rail")&&!document.querySelector('[data-tile="0"]').classList.contains("action-medallion"),"Full-bleed GIFs do not get card overlays");
  for(const mode of ["advanced","widget"])for(const source of ["emoji","image","gif"]){
   const macro=mode==="widget"?{...make(0),widgetGraphic:"custom_icon"}:{...emptyMacro(),url:"https://example.com/"};
   Object.assign(macro,{name:"Test",icon:"★",iconSource:source,visualFit:"cover",imagePreview:"data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"});
   if(source!=="emoji"){macro.iconImage="i0000000000000001";macro.iconFormat=source==="gif"?"gif":"rgb565a8";macro.iconSize=160}
   reset({pageCount:1,macros:[macro]});selectTile(0);
   const tile=document.querySelector('[data-tile="0"]');
   check(tile.classList.contains("visual-cover")&&!!tile.querySelector(".visual-background")&&!tile.querySelector(".widget-graphic"),`${mode} ${source} visual fills behind content`);
   check(document.querySelector('[data-k="visualFit"]').value==="cover"&&!document.querySelector('[data-k="visualFit"]').closest(".field").classList.contains("hidden"),`${mode} ${source} exposes background placement`);
   check(document.querySelector('[data-k="widgetLayout"]').closest(".field").classList.contains("hidden")===(mode!=="widget"),`${mode} ${source} shows card styles only when applicable with a fill visual`);
   change("visualFit","contain");
   check(!document.querySelector('[data-tile="0"]').classList.contains("visual-cover"),`${mode} ${source} returns to contained visual`);
  }
  const padded=document.createElement("canvas");padded.width=160;padded.height=160;
  const paddedContext=padded.getContext("2d");paddedContext.fillStyle="#ff0000";paddedContext.fillRect(0,40,160,80);
  const paddedSource=padded.toDataURL("image/png"),cropped=await croppedCoverPreview(paddedSource),croppedImage=new Image();
  await new Promise((resolve,reject)=>{croppedImage.onload=resolve;croppedImage.onerror=reject;croppedImage.src=cropped});
  check(croppedImage.naturalWidth===160&&croppedImage.naturalHeight===80,"Cover preview trims transparent padding without changing the saved asset");
  const transparent=document.createElement("canvas");transparent.width=8;transparent.height=8;
  const transparentSource=transparent.toDataURL("image/png");
  check(await croppedCoverPreview(transparentSource)===transparentSource,"Entirely transparent assets remain unchanged");
  reset({pageCount:1,macros:[{...emptyMacro(),url:"https://example.com/",iconSource:"image",iconFormat:"rgb565a8",iconSize:160,iconImage:"i0000000000000001",imagePreview:paddedSource,visualFit:"cover"}]});
  await croppedCoverPreview(paddedSource);await Promise.resolve();
  check(document.querySelector('[data-tile="0"] .visual-background').src===cropped,"Cover preview displays the cropped image");
  check(cfg.macros[0].imagePreview===paddedSource,"Cover preview does not rewrite the saved visual");
  for(const mode of ["advanced","widget","pc_macro","wake_on_lan","smart_home","sound"]){
   reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);
   if(mode==="wake_on_lan"){change("actionMode","widget");chooseGroup("pc");change("widget","wake_on_lan")}
   else change("actionMode",mode);
   const selector=document.querySelector('[data-k="widgetLayout"]');
   check(selector.closest('[data-section="appearance"]')&&!selector.closest(".field").classList.contains("hidden")&&selector.options.length===3,"All styles are available for "+mode);
  }
  for(const widget of [false,true])for(const style of ["classic","panel","layered"]){
   let saved;
   globalThis.fetch=async (url,options={})=>{
    if(url==="/api/config"&&options.method==="POST"){saved=JSON.parse(options.body);return new Response('{"ok":true}',{status:200})}
    if(url==="/api/config")return new Response(JSON.stringify(saved),{status:200});
    if(url==="/api/status")return new Response('{"ssid":"test"}',{status:200});
    if(url==="/api/time")return new Response("{}",{status:200});
    throw Error("Unexpected request "+url);
   };
   reset({pageCount:1,macros:[widget?make(0):{...emptyMacro(),name:"Action",url:"https://example.com/"}]});selectTile(0);
   change("widgetLayout",style);await document.querySelector("#save").onclick();
   check(saved?.macros[0].widgetLayout===style,`${widget?"Widget":"Action"} ${style} is sent on save`);
   await init();selectTile(0);
   check(document.querySelector('[data-k="widgetLayout"]').value===style,`${widget?"Widget":"Action"} ${style} survives reload`);
  }
  globalThis.fetch=fetchOriginal;
  reset({cols:3,rows:3,pageCount:1,macros:[make(0),...Array.from({length:8},emptyMacro)]});selectTile(0);
  change("spanCols","2");change("spanRows","2");
  let large=document.querySelector('[data-tile="0"]'),neighbor=document.querySelector('[data-tile="2"]');
  check(cfg.macros[0].spanCols===2&&cfg.macros[0].spanRows===2&&![1,3,4].some(i=>document.querySelector(`[data-tile="${i}"]`)),"A 2 by 2 tile covers the requested positions");
  check(large.getBoundingClientRect().width>neighbor.getBoundingClientRect().width*1.9&&large.getBoundingClientRect().height>neighbor.getBoundingClientRect().height*1.9,"Preview uses the enlarged area");
  check(widgetDimensions(cfg.macros[0],3,3).width===304&&widgetDimensions(cfg.macros[0],3,3).height===248,"Preview uses the device's spanned pixel dimensions");
  document.querySelector("#hideUnconfigured").checked=true;document.querySelector("#hideUnconfigured").dispatchEvent(new Event("change"));
  check(!document.querySelector("#hideUnconfigured").checked,"Compacting is rejected while a tile is enlarged");
  const spanTransfer=new DataTransfer();
  large.dispatchEvent(new DragEvent("dragstart",{bubbles:true,dataTransfer:spanTransfer}));
  neighbor.dispatchEvent(new DragEvent("drop",{bubbles:true,dataTransfer:spanTransfer}));
  check(cfg.macros[0].spanCols===2&&!cfg.macros[2].spanCols,"Moving an enlarged tile past a page edge is rejected");
  let spanSaved;
  globalThis.fetch=async (url,options={})=>{
   if(url==="/api/config"&&options.method==="POST"){spanSaved=JSON.parse(options.body);return new Response('{"ok":true}',{status:200})}
   if(url==="/api/config")return new Response(JSON.stringify(spanSaved),{status:200});
   if(url==="/api/status")return new Response('{"ssid":"test"}',{status:200});
   if(url==="/api/time")return new Response("{}",{status:200});
   throw Error("Unexpected request "+url);
  };
  await document.querySelector("#save").onclick();
  check(spanSaved?.macros[0].spanCols===2&&spanSaved.macros[0].spanRows===2&&spanSaved.macros[1].name==="","Saving preserves the size and covered positions");
  await init();selectTile(0);
  check(document.querySelector('[data-k="spanCols"]').value==="2"&&document.querySelector('[data-k="spanRows"]').value==="2"&&!document.querySelector('[data-tile="4"]'),"Reload restores the enlarged tile");
  globalThis.fetch=fetchOriginal;
  change("spanRows","1");change("spanCols","1");
  check(!!document.querySelector('[data-tile="1"]')&&!!document.querySelector('[data-tile="3"]')&&!cfg.macros[0].spanCols&&!cfg.macros[0].spanRows,"Shrinking restores covered slots");
  reset({cols:3,rows:3,pageCount:1,macros:[make(0),make(1)]});selectTile(0);change("spanCols","2");
  check(!cfg.macros[0].spanCols&&cfg.macros[1].name==="W1"&&document.querySelector('[data-k="spanCols"]').value==="1","Resizing cannot cover another tile");
  reset({cols:3,rows:3,pageCount:1,macros:[emptyMacro(),make(1)]});selectTile(1);change("spanCols","2");
  const colsControl=document.querySelector("#cols");colsControl.value="2";colsControl.dispatchEvent(new Event("change"));
  check(colsControl.value==="3"&&cfg.cols===3&&cfg.macros[1].spanCols===2,"Grid changes cannot strand an enlarged tile");
  reset({cols:3,rows:3,pageCount:1,macros:[...Array.from({length:8},emptyMacro),make(8)]});selectTile(8);change("spanRows","2");
  check(!cfg.macros[8].spanRows,"Tiles cannot cross a page boundary");
  reset({pageCount:1,macros:[emptyMacro()]});selectTile(0);change("actionMode","widget");
  check(cfg.macros[0].widgetLayout==="classic"&&document.querySelector('[data-k="widgetLayout"]').value==="classic","A new tile defaults to Classic");
  const legacy={...make(0)};delete legacy.widgetLayout;reset({pageCount:1,macros:[legacy]});selectTile(0);
  check(document.querySelector('[data-k="widgetLayout"]').value==="panel","An existing tile without a style retains Modern");
  check(!document.querySelector(".device-toolbar")&&!document.querySelector("#tileBudget"),"Device preview has no header or speculative budget");
  const primaryColors=new Set(),secondaryColors=new Set();
  for(const theme of ["dark","light","ocean","forest","violet","ember"]){
   const control=document.querySelector("#siteTheme");control.value=theme;control.dispatchEvent(new Event("change"));
   check(document.body.dataset.siteTheme===theme && document.body.classList.contains("light")===(theme==="light") && localStorage.getItem("goopad-site-theme")===theme,"Website theme persists: "+theme);
   primaryColors.add(getComputedStyle(document.querySelector("#save")).backgroundColor);
   secondaryColors.add(getComputedStyle(document.querySelector("#backup")).backgroundColor);
  }
  check(primaryColors.size===6 && secondaryColors.size===6,"Website themes recolor primary and secondary buttons");
  check(!document.querySelector(".layout-settings").open && !!document.querySelector(".layout-settings #cols"),"Layout settings start collapsed without removing controls");
  check(!!document.querySelector(".layout-settings #deviceTheme")&&!!document.querySelector(".layout-settings #backgroundUpload")&&!document.querySelector("#settingsView #deviceTheme"),"Device themes and custom backgrounds are in Layout settings");
  document.querySelector(".layout-settings summary").click();
  check(document.querySelector(".layout-settings").open && choice("cols","3").checkVisibility(),"Layout settings can be opened");
  document.querySelector(".layout-settings summary").click();
  for (const page of [0,1,2]) {
   reset({}); scrollPreviewPage(page); document.querySelector("#addPage").click();
   check(cfg.pageCount === 4 && currentPreviewPage() === page+1, "Insertion/selection after current page");
   check(selectedTile===-1 && !document.querySelector('[data-k="actionMode"]'), "New page leaves every tile unselected");
   check(cfg.macros[(page+1)*6].name === "", "Inserted page starts empty");
   if (page<2) check(cfg.macros[(page+2)*6].name === "W"+((page+1)*6), "Insertion preserves later slots");
   document.querySelector("#removePage").click();
   check(cfg.pageCount === 3 && cfg.macros[6].name === "W6", "Removing an empty inserted page");
  }
  check(!document.querySelector("#pageTemplate,#insertTemplate"),"Premade app page controls are absent");
  reset({}); scrollPreviewPage(1);
  globalThis.confirm = () => false; document.querySelector("#removePage").click();
  check(cfg.pageCount === 3 && cfg.macros.length === 18, "Cancel deletion preserves page");
  globalThis.confirm = () => true; document.querySelector("#removePage").click();
  check(cfg.pageCount === 2 && cfg.macros[6].name === "W12", "Confirmed deletion shifts later pages");
  reset({cols:4,rows:3,pageCount:2,macros:Array.from({length:24},(_,i)=>make(i))});
  const before = JSON.stringify(cfg); document.querySelector("#addPage").click();
  check(JSON.stringify(cfg) === before, "Overflow must not discard configured tiles");
  for(let i=20;i<24;i++) cfg.macros[i]=emptyMacro();
  document.querySelector("#addPage").click();
  check(cfg.macros.length===32 && cfg.macros[31].name==="W19", "Partial final page preserves configured slots");
  check(document.querySelector('[data-tile="32"]').disabled, "Positions beyond 32 disabled");
  document.querySelector("#hideUnconfigured").checked=true; preview();
  check(document.querySelector("#addPage").disabled && document.querySelector("#removePage").disabled, "Compact mode prevents ambiguous page edits");
  reset({pageCount:1,macros:[]});
  check(document.querySelector("#removePage").disabled, "At least one page remains");
  reset({macros:[make(0)]});
  const transfer=new DataTransfer(), source=document.querySelector('[data-tile="0"]'), arrow=document.querySelector("#nextPage");
  source.dispatchEvent(new DragEvent("dragstart",{bubbles:true,dataTransfer:transfer}));
  arrow.dispatchEvent(new DragEvent("dragenter",{bubbles:true,dataTransfer:transfer}));
  arrow.dispatchEvent(new DragEvent("dragover",{bubbles:true,dataTransfer:transfer}));
  await new Promise(resolve=>setTimeout(resolve,700));
  check(currentPreviewPage()===1, "Drag-hover arrow navigates");
  arrow.dispatchEvent(new DragEvent("dragleave",{bubbles:true,dataTransfer:transfer}));
  document.querySelector('[data-tile="6"]').dispatchEvent(new DragEvent("drop",{bubbles:true,dataTransfer:transfer}));
  check(cfg.macros[6].name==="W0" && !cfg.macros[0].name && selectedTile===6, "Cross-page empty-slot move");
  await new Promise(resolve=>setTimeout(resolve,650));
  check(currentPreviewPage()===1, "Drag-hover timer stops after drop");
  reset({cols:2,rows:2,pageCount:2,macros:[emptyMacro(),make(1),emptyMacro(),make(3)]});
  const tilePosition=document.querySelector('[data-tile="3"]').getBoundingClientRect();
  document.querySelector("#hideEmptyTiles").checked=true; document.querySelector("#hideEmptyTiles").dispatchEvent(new Event("change"));
  const empty=document.querySelector('[data-tile="0"]'),hiddenPosition=document.querySelector('[data-tile="3"]').getBoundingClientRect();
  check(empty.style.background==="transparent" && getComputedStyle(empty).borderWidth==="0px", "Hidden empty tile has no box");
  check(hiddenPosition.x===tilePosition.x && hiddenPosition.y===tilePosition.y && document.querySelector("#preview").children.length===2, "Hiding preserves slots and pages");
  check(!document.querySelector("#addPage").disabled && !document.querySelector("#removePage").disabled, "Hiding does not prevent page edits");
  empty.click();
  check(selectedTile===0 && !!document.querySelector('[data-k="actionMode"]'), "Hidden space remains editable");
  const dropSource=document.querySelector('[data-tile="3"]'),dropTransfer=new DataTransfer();
  dropSource.dispatchEvent(new DragEvent("dragstart",{bubbles:true,dataTransfer:dropTransfer}));
  document.querySelector('[data-tile="0"]').dispatchEvent(new DragEvent("drop",{bubbles:true,dataTransfer:dropTransfer}));
  check(cfg.macros[0].name==="W3" && !macroConfigured(cfg.macros[3]), "Hidden space accepts a moved tile");
  document.querySelector("#hideEmptyTiles").checked=false; preview();
  check(document.querySelector('[data-tile="3"]').style.background!=="transparent", "Turning hiding off restores empty boxes");
  const website={...emptyMacro(),actionMode:"pc_open_url",pcTarget:"https://example.com/test"};
  reset({pageCount:1,macros:[emptyMacro(),emptyMacro()]});selectTile(0);
  check(document.querySelector('[data-k="actionMode"]').value==="" && document.querySelector('[data-k="actionMode"] option[value=""]').disabled,"New tile prompts for an action");
  check([...document.querySelectorAll("[data-mode-field],[data-section]")].every(field=>field.classList.contains("hidden")) && document.querySelector('[data-k="name"]').closest(".field").classList.contains("hidden") && document.querySelector(".tile-action-buttons").classList.contains("hidden"),"New tile hides action and editing fields until a choice is made");
  change("actionMode","advanced");
  check(!document.querySelector('[data-mode-field="advanced"]').classList.contains("hidden") && !document.querySelector('[data-k="name"]').closest(".field").classList.contains("hidden"),"Choosing a custom web request reveals its fields");
  check(!document.querySelector('[data-k="outline"]').checked && document.querySelector('[data-tile="0"]').style.borderWidth==="1px","Empty placeholder has a subtle border without changing the configured outline");
  change("url","https://example.com/test");render();
  check(document.querySelector('[data-tile="0"]').style.borderWidth==="0px","Configured tiles preserve the chosen no-outline style");
  check(document.querySelector('[data-k="actionMode"]').value==="advanced" && document.querySelector('[data-k="url"]').value==="https://example.com/test","Selected custom action survives rerender");
  document.querySelector("[data-delete]").click();
  check(document.querySelector('[data-k="actionMode"]').value==="" && document.querySelector('[data-mode-field="advanced"]').classList.contains("hidden"),"Clearing a tile restores the action prompt");
  selectTile(1);selectTile(0);
  check(document.querySelector('[data-k="actionMode"]').value==="" && cfg.macros[0].actionMode==="simple_link","Switching tiles preserves an unselected action");
  check(!document.querySelector('[data-k="actionMode"]').closest("details") && ["icon","appearance","text","sound"].every(id=>!document.querySelector(`[data-section="${id}"]`).open),"Action is visible and optional sections start collapsed");
  check(!!document.querySelector('[data-section="icon"] [data-k="iconSource"]') && !!document.querySelector('[data-section="appearance"] [data-k="shape"]') && !!document.querySelector('[data-section="text"] [data-k="font"]') && !!document.querySelector('[data-section="sound"] [data-k="sound"]'),"Editing controls are grouped by purpose");
  document.querySelector('[data-section="icon"] summary').click();render();
  check(document.querySelector('[data-section="icon"]').open && !document.querySelector('[data-section="text"]').open,"Open sections survive rerendering the same tile");
  change("actionMode","widget");
  check(!document.querySelector('[data-mode-field="widget"]').classList.contains("hidden") && document.querySelector('[data-section="sound"]').classList.contains("hidden") && document.querySelector(".tile-action-buttons").classList.contains("hidden"),"Widget setup is visible without irrelevant sound and test actions");
  change("widgetGraphic","custom_icon");
  check(!document.querySelector('[data-k="iconSource"]').closest(".field").classList.contains("hidden"),"Widget icon controls appear within the open icon section");
  change("actionMode","sound");
  check(document.querySelector('[data-section="sound"]').open && !document.querySelector('[data-section="sound"]').classList.contains("hidden"),"Sound-only mode opens its required sound control");
  change("sound","file");
  check(!document.querySelector("[data-sound-upload]").classList.contains("hidden"),"Sound upload remains available");
  selectTile(1);
  check(["icon","appearance","text","sound"].every(id=>!document.querySelector(`[data-section="${id}"]`).open),"Another tile starts with optional sections collapsed");
  reset({pageCount:1,macros:[website],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  const websiteFields=document.querySelector('[data-mode-field="pc_open_url"]');
  check(!websiteFields.querySelector('[data-k="pcAddress"],[data-k="secret"]') && !!websiteFields.querySelector('[data-k="pcTarget"]'), "Website editor asks only for website");
  check(macroConfigured(website), "Draft website remains visible before save");
  prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.20:8787/open" && website.secret===cfg.companionToken && website.method==="POST" && JSON.parse(website.body).url===website.pcTarget, "Website uses verified HTTPS pairing and default port");
  reset({pageCount:1,macros:[emptyMacro()],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  check(!document.querySelector('[data-k="actionMode"] option[value="pc_open_url"]'),"New PC websites are configured inside the PC builder");
  change("actionMode","widget");chooseGroup("pc");change("widget","wake_on_lan");
  check(document.querySelector('[data-k="widget"] option[value="wake_on_lan"]').textContent==="Wake-on-LAN","PC option uses the Wake-on-LAN label");
  check(!document.querySelector('[data-mode-field="wake_on_lan"]').classList.contains("hidden"),"Wake-on-LAN displays the guided MAC field");
  check(document.querySelector('[data-k="actionMode"]').value==="widget"&&document.querySelector("[data-widget-category]").value==="pc"&&document.querySelector('[data-k="widget"]').value==="wake_on_lan"&&!document.querySelector(".tile-action-buttons").classList.contains("hidden"),"Wake-on-LAN remains an actionable PC option");
  change("wakeMac","02-11-22-33-44-aa");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].wakeMac==="02:11:22:33:44:AA" && cfg.macros[0].url==="" && macroConfigured(cfg.macros[0]),"Wake MAC is normalized and configured without a web request");
  let wakeSaved;
  globalThis.fetch=async(url,options)=>url==="/api/config"?(wakeSaved=JSON.parse(options.body),new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(wakeSaved?.macros[0].wakeMac==="02:11:22:33:44:AA" && wakeSaved.macros[0].actionMode==="wake_on_lan","Saving retains the guided wake action");
  cfg.macros[0]=structuredClone(wakeSaved.macros[0]);selectTile(0);
  check(document.querySelector('[data-k="wakeMac"]').value==="02:11:22:33:44:AA"&&document.querySelector('[data-k="widget"]').value==="wake_on_lan"&&document.querySelector("[data-widget-category]").value==="pc","Saved wake action reopens under PC");
  globalThis.fetch=fetchOriginal;
  for(const mac of ["","00:00:00:00:00:00","ff:ff:ff:ff:ff:ff","01:11:22:33:44:55","02:11-22:33:44:55","02:11:22:33:44","02:11:22:33:44:gg"]){
   cfg.macros[0].wakeMac=mac;
   let invalid=false;try{prepareMacroAction(cfg.macros[0],0)}catch{invalid=true}
   check(invalid,"Invalid wake MAC is rejected: "+mac);
  }
  cfg.macros[0].actionMode="simple_link";prepareMacroAction(cfg.macros[0],0);
  check(!("wakeMac" in cfg.macros[0]),"Changing action removes the wake address");
  reset({pageCount:1,macros:[wakeSaved.macros[0]]});selectTile(0);change("widget","pc_monitor");
  check(cfg.macros[0].actionMode==="widget"&&cfg.macros[0].widget==="pc_monitor"&&document.querySelector('[data-mode-field="wake_on_lan"]').classList.contains("hidden"),"Switching a saved wake action back to a PC widget works");
  reset({});scrollPreviewPage(2);
  let pageSave;
  globalThis.fetch=async(url,options)=>url==="/api/config"?(pageSave=JSON.parse(options.body),new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(pageSave?.pageCount===3 && currentPreviewPage()===2,"Saving preserves the editor preview page");
  globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[emptyMacro()],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  change("actionMode","smart_home");
  const smartType=document.querySelector("[data-smart-home-type]");smartType.value="homekit";smartType.dispatchEvent(new Event("change"));
  change("homekitName","Desk light");
  prepareMacroAction(cfg.macros[0],0);
  const homekitId=cfg.macros[0].homekitId;
  check(/^s[0-9a-f]{14}$/.test(homekitId) && cfg.macros[0].homekitName==="Desk light" && cfg.macros[0].url==="" && macroConfigured(cfg.macros[0]),"HomeKit switch has a stable ID and no web request");
  check(!!document.querySelector('[data-tile="0"] .homekit-indicator'),"HomeKit tile preview shows an example state indicator");
  cfg.macros[0].homekitName="Renamed light";prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].homekitId===homekitId,"Renaming a switch preserves its identity");
  cfg.macros[0].homekitName="";cfg.macros[0].name="";
  let missingHomekitName=false;try{prepareMacroAction(cfg.macros[0],0)}catch{missingHomekitName=true}
  check(missingHomekitName,"A switch requires a name");
  cfg.macros[0].actionMode="simple_link";prepareMacroAction(cfg.macros[0],0);
  check(!cfg.macros[0].homekitId && !cfg.macros[0].homekitName,"Changing action removes the old switch identity");
  reset({pageCount:1,macros:[emptyMacro()],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  change("actionMode","pc_macro");
  const starter=document.querySelector("[data-pc-preset]");starter.value="open";starter.dispatchEvent(new Event("change"));
  const websiteInput=document.querySelector('[data-step-fields] [data-value]');websiteInput.value="https://example.com/test";websiteInput.dispatchEvent(new Event("input"));
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].url==="https://192.0.2.20:8787/run" && JSON.parse(cfg.macros[0].body).steps[0].value==="https://example.com/test","PC builder sends an open-website step");
  let sharedSave;
  globalThis.fetch=async(url,options={})=>url==="/api/config"?(sharedSave=JSON.parse(options.body),new Response("{}")):fetchOriginal(url,options);
  cfg.macros.push({...make(1),widget:"now_playing"},{...make(2),widget:"pc_monitor"});
  await document.querySelector("#save").onclick();
  check(sharedSave.macros[0].sharedPc===true&&sharedSave.macros[0].url==="/run"&&sharedSave.macros[0].secret==="","PC macro stores a shared pairing reference");
  check(sharedSave.macros.slice(1).every(m=>m.sharedPc===true&&m.url==="/monitor"&&m.secret===""),"PC widgets store shared pairing references");
  check(!JSON.stringify(sharedSave.macros).includes(cfg.companionToken),"Shared token is stored only at the configuration root");
  check(JSON.stringify(cfg.macros.map(savedMacro)).length+100<JSON.stringify(cfg.macros.map(({imagePreview,pcSteps,tileName,sharedPc,...macro})=>macro)).length,"Shared pairing reduces saved configuration size");
  cfg=sharedSave;cfg.companionToken="z".repeat(32);
  cfg.macros.forEach(prepareMacroAction);
  check(cfg.macros.every(m=>m.secret===cfg.companionToken),"Shared PC tiles use updated pairing when edited after reload");
  const unrelated={...emptyMacro(),actionMode:"advanced",url:cfg.companionAddress+"/run",secret:cfg.companionToken};
  check(!savedMacro(unrelated).sharedPc&&savedMacro(unrelated).secret===cfg.companionToken,"Custom actions keep their independent token");
  globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[emptyMacro()],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  change("actionMode","pc_macro");
  const resetStarter=document.querySelector("[data-pc-preset]");resetStarter.value="open";resetStarter.dispatchEvent(new Event("change"));
  const resetInput=document.querySelector('[data-step-fields] [data-value]');resetInput.value="https://example.com/test";resetInput.dispatchEvent(new Event("input"));
  prepareMacroAction(cfg.macros[0],0);
  cfg.macros[0].pcSteps[0].value="https://example.com/"+("a".repeat(180));
  let longWebsiteRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{longWebsiteRejected=true}
  check(longWebsiteRejected,"PC website steps reject links the Companion cannot accept");
  reset({pageCount:1,macros:[website],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  website.pcAddress="192.0.2.30:8787";website.secret="y".repeat(32);
  prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.20:8787/open" && website.secret===cfg.companionToken, "Settings override legacy website pairing");
  cfg.companionAddress="";cfg.companionToken="";website.secret="y".repeat(32);prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.30:8787/open" && website.secret==="y".repeat(32), "Legacy website address uses HTTPS with the paired certificate");
  check(!savedMacro(website).sharedPc&&savedMacro(website).secret==="y".repeat(32),"Legacy per-tile pairing remains independent");
  delete website.pcAddress;website.secret="";
  let rejected=false;try{prepareMacroAction(website,0)}catch{rejected=true}
  check(rejected, "Missing pairing must fail explicitly");
  for(const [address,cert] of [["http://192.0.2.20:8787",certificate],["192.0.2.20",""],["192.0.2.20",certificate+"\n-----BEGIN PRIVATE KEY-----"]]){
   let rejected=false;try{pcConnection(address,"x".repeat(32),cert)}catch{rejected=true}
   check(rejected,"HTTP or invalid/private TLS material cannot be paired");
  }
  document.querySelector("#companionPairing").value=JSON.stringify({address:"https://192.0.2.20:8787",token:"x".repeat(32),certificate});
  document.querySelector("#applyPairing").click();
  check(cfg.companionAddress==="https://192.0.2.20:8787" && cfg.companionCertificate===certificate && document.querySelector("#companionPairing").value==="", "Secure bundle updates draft pairing and clears private pasted code");
  check(!document.querySelector("#companionAddress,#companionToken,#companionCertificate"), "Pairing has no manual address, token or certificate controls");
  const pairedDraft=JSON.stringify(cfg);
  document.querySelector("#companionPairing").value=JSON.stringify({address:"http://192.0.2.20:8787",token:"x".repeat(32),certificate});
  document.querySelector("#applyPairing").click();
  check(JSON.stringify(cfg)===pairedDraft,"Invalid secure code preserves existing pairing");
  document.querySelector("#companionPairing").value="";
  setForm();
  let secureSaved;
  globalThis.fetch=async(url,options)=>url==="/api/config"
   ? (secureSaved=JSON.parse(options.body),new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(secureSaved?.companionCertificate===certificate && secureSaved.companionAddress==="https://192.0.2.20:8787" && secureSaved.macros[0].url==="/open" && secureSaved.macros[0].sharedPc===true,"Save persists certificate and shared HTTPS macro");
  globalThis.confirm=()=>false;
  let triggered=false;
  globalThis.fetch=async(url,options)=>url==="/api/trigger"?(triggered=true,new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("[data-test-action]").onclick();
  check(!triggered,"Cancel prevents real saved action execution");
  globalThis.confirm=confirmOriginal;globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[{...emptyMacro(),icon:"⚙",sound:"chime"}]});selectTile(0);
  let emojiUploads=0,emojiPayload;
  globalThis.fetch=async (url,options)=>{
   if(String(url).startsWith("/api/icon-match?"))return new Response('{"key":null,"next":null}',{status:200});
   if(String(url).startsWith("/api/icon?")){emojiUploads++;emojiPayload=options.body;return new Response("{}",{status:200})}
   return fetchOriginal(url,options)
  };
  await upgradeEmojiIcons();
  const emoji=cfg.macros[0];
  emojiPayload=pendingIconAssets.get(emoji.iconImage)?.body;
  check(emoji.iconSource==="emoji" && emoji.iconFormat==="rgb565a8" && emoji.iconSize===160 && !!emoji.emojiImageKey && emojiPayload instanceof Uint8Array && emojiUploads===0, "Full-color emoji is staged without an upload");
  const alpha=emojiPayload[0]===71&&emojiPayload[1]===80&&emojiPayload[2]===82&&emojiPayload[3]===49
   ? Array.from({length:(emojiPayload.length-6)/5},(_,i)=>emojiPayload[10+i*5])
   : Array.from(emojiPayload.slice(160*160*2));
  check(alpha.some(value=>value===0) && alpha.some(value=>value>0), "Emoji upload preserves transparency and visible glyph pixels");
  await upgradeEmojiIcons();
  await savePendingIconAssets();
  check(emojiUploads===1, "Unchanged saved emoji image is reused");
  change("emojiRender","symbol");
  check(!emoji.iconImage && !emoji.imagePreview, "Switching to built-in symbol removes raster image");
  const weatherEmoji={...emptyMacro(),actionMode:"widget",widget:"weather",widgetGraphic:"custom_icon",icon:"☀",emojiRender:"image"};
  cfg.macros.push(weatherEmoji);await upgradeEmojiIcons();
  check(emojiUploads===1 && !!weatherEmoji.iconImage,"Custom weather emoji is prepared without an upload");
  await savePendingIconAssets();
  check(emojiUploads===2,"Custom weather emoji uploads when saved");
  weatherEmoji.widgetGraphic="text";delete weatherEmoji.iconImage;await upgradeEmojiIcons();
  check(emojiUploads===2,"Automatic weather icon does not upload a selected emoji");
  cfg.macros.pop();
  const imageCanvas=document.createElement("canvas");imageCanvas.width=imageCanvas.height=2;
  imageCanvas.getContext("2d").fillRect(0,0,2,2);
  const imageBlob=await new Promise(resolve=>imageCanvas.toBlob(resolve,"image/png"));
  reset({pageCount:1,macros:[{...emptyMacro(),actionMode:"sound",sound:"chime",iconSource:"image"}]});selectTile(0);
  const beforeSave=emojiUploads;
  check(await uploadIconImage(imageBlob,0,cfg.macros[0]),"Image can be staged");
  const stagedKey=cfg.macros[0].iconImage;
  check(emojiUploads===beforeSave&&pendingIconAssets.has(stagedKey),"Image selection does not upload");
  cfg.macros.push({...cfg.macros[0]});
  let configPosts=0,failUpload=true;
  globalThis.fetch=async (url,options={})=>{
   if(String(url).startsWith("/api/icon-match?"))return new Response('{"key":null,"next":null}',{status:200});
   if(String(url).startsWith("/api/icon?")){emojiUploads++;return new Response(failUpload?"SD full":"{}",{status:failUpload?500:200})}
   if(url==="/api/config"){configPosts++;return new Response("{}",{status:200})}
   throw Error("Unexpected request "+url)
  };
  await document.querySelector("#save").onclick();
  check(configPosts===0&&pendingIconAssets.has(stagedKey)&&document.querySelector("#saveStatus").textContent.includes("SD full"),"Failed upload prevents config save and remains retryable");
  failUpload=false;await document.querySelector("#save").onclick();
  check(configPosts===1&&emojiUploads===beforeSave+2&&!pendingIconAssets.has(stagedKey),"Retry uploads the shared asset once and saves configuration");
  await document.querySelector("#save").onclick();
  check(configPosts===2&&emojiUploads===beforeSave+2,"Subsequent save does not reupload the asset");
  reset({pageCount:1,macros:[{...emptyMacro(),actionMode:"sound",sound:"chime",iconSource:"image"},{...emptyMacro(),actionMode:"sound",sound:"chime",iconSource:"image"}]});selectTile(0);
  check(await uploadIconImage(imageBlob,0,cfg.macros[0])&&await uploadIconImage(imageBlob,1,cfg.macros[1])&&cfg.macros[0].iconImage===cfg.macros[1].iconImage,"Identical images share one staged asset");
  let reusedConfig,reuseSearches=0;
  const savedKey="i0000000000000002",uploadsBeforeReuse=emojiUploads;
  globalThis.fetch=async(url,options={})=>{
   if(String(url).startsWith("/api/icon-match?")){
    reuseSearches++;return new Response(JSON.stringify(reuseSearches===1?{key:null,next:16}:{key:savedKey,next:null}),{status:200})
   }
   if(String(url).startsWith("/api/icon?")){emojiUploads++;return new Response("{}",{status:200})}
   if(url==="/api/config"){reusedConfig=JSON.parse(options.body);return new Response("{}",{status:200})}
   throw Error("Unexpected request "+url)
  };
  await document.querySelector("#save").onclick();
  check(reuseSearches===2&&emojiUploads===uploadsBeforeReuse&&reusedConfig?.macros.every(m=>m.iconImage===savedKey),"Saved images are found across pages and reused without uploading");
  await uploadIconImage(imageBlob,0,cfg.macros[0]);
  globalThis.fetch=async url=>{
   if(String(url).startsWith("/api/icon-match?"))return new Response("SD unavailable",{status:500});
   throw Error("Unexpected request "+url)
  };
  await document.querySelector("#save").onclick();
  check(document.querySelector("#saveStatus").textContent.includes("SD unavailable")&&pendingIconAssets.has(cfg.macros[0].iconImage),"Asset lookup errors leave the draft retryable and visible");
  globalThis.fetch=async (url,options={})=>{
   if(String(url).startsWith("/api/icon-match?"))return new Response('{"key":null,"next":null}',{status:200});
   if(String(url).startsWith("/api/icon?")){emojiUploads++;return new Response("{}",{status:200})}
   if(url==="/api/config"){configPosts++;return new Response("{}",{status:200})}
   throw Error("Unexpected request "+url)
  };
  const gifBytes=Uint8Array.from(atob("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"),c=>c.charCodeAt(0));
  const canonical=canonicalGif(gifBytes);
  check(canonicalGif(canonical).every((byte,index)=>byte===canonical[index])&&canonical.length===canonicalGif(canonical).length,"GIF matching uses the device's stable normalized form");
  reset({pageCount:1,macros:[{...emptyMacro(),actionMode:"sound",sound:"chime",iconSource:"gif"}]});selectTile(0);
  const beforeGif=emojiUploads;
  check(await uploadGif(new File([gifBytes],"sample.gif",{type:"image/gif"}),0,cfg.macros[0])&&emojiUploads===beforeGif,"GIF selection does not upload");
  await document.querySelector("#save").onclick();
  check(emojiUploads===beforeGif+1&&configPosts===3,"GIF uploads on Save");
  const beforeBackground=emojiUploads;
  await uploadBackground({files:[imageBlob],disabled:false});
  check(emojiUploads===beforeBackground&&pendingIconAssets.has(cfg.backgroundImage),"Background selection does not upload");
  await document.querySelector("#save").onclick();
  check(emojiUploads===beforeBackground+1&&configPosts===4,"Background uploads on Save");
  globalThis.fetch=async url=>{
   if(String(url).startsWith("/api/saved-icons"))return new Response(JSON.stringify({assets:[{key:stagedKey,format:"gif",size:160}],next:null}),{status:200});
   if(url===`/api/icon?key=${stagedKey}&format=gif&size=160`)return new Response(gifBytes,{status:200});
   throw Error("Unexpected request "+url)
  };
  cfg.macros[0].iconSource="image";selectTile(0);
  await showSavedAssets(document.querySelector(".macro"),cfg.macros[0],0);
  document.querySelector("[data-assets-results] button").click();
  check(cfg.macros[0].iconImage===stagedKey&&cfg.macros[0].iconSource==="gif","Saved visuals are selectable without uploading");
  const gif={...emptyMacro(),iconSource:"gif"};cfg.macros=[gif];selectedTile=0;render();
  uploadMessage(0,"Uploading animation to SD…");
  const notice=document.querySelector('[data-visual="gif"] [data-upload-status]'),description=notice.previousElementSibling,noticeStyle=getComputedStyle(notice);
  const progressColor=noticeStyle.color;
  check(progressColor!==getComputedStyle(description).color && Number(noticeStyle.fontWeight)>=700 && noticeStyle.borderTopWidth==="1px", "GIF progress is visually distinct from its description");
  uploadMessage(0,"GIF upload failed.",true);
  check(getComputedStyle(notice).color!==progressColor, "Upload errors have a distinct color");
  globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[{...emptyMacro(),icon:"⚙",emojiRender:"symbol",sound:"chime",textColor:"FEDCBA"}]});selectTile(0);
  check(!cfg.macros[0].iconColor && document.querySelector('[data-k="iconColor"]').value.toUpperCase()==="#FEDCBA","Existing symbols default to the label color");
  change("iconColor","#12AB34");
  check(cfg.macros[0].iconColor==="12AB34" && document.querySelector('[data-tile="0"] .ico').style.color==="rgb(18, 171, 52)","Custom symbol color updates independently in preview");
  let colorSaved;
  globalThis.fetch=async(url,options)=>url==="/api/config"
   ? (colorSaved=JSON.parse(options.body),new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(colorSaved?.macros[0].iconColor==="12AB34" && colorSaved.macros[0].textColor==="FEDCBA","Save persists icon color independently of label color");
  globalThis.fetch=fetchOriginal;
  document.querySelector("[data-reset-icon-color]").click();
  check(!cfg.macros[0].iconColor && !document.querySelector('[data-tile="0"] .ico').style.color,"Reset restores the original symbol color");
  const fa={...emptyMacro(),sound:"chime",iconSource:"fontawesome",faIcon:"solid:star",iconImage:"i123",imagePreview:"data:image/png;base64,AA==",textColor:"123456"};
  reset({pageCount:1,macros:[fa]});selectTile(0);
  check(document.querySelector('[data-k="iconColor"]').value.toUpperCase()==="#FFFFFF" && !!document.querySelector('[data-tile="0"] img'),"Existing Font Awesome icons retain their original color");
  change("iconColor","#FF8800");
  check(document.querySelector('[data-tile="0"] .icon-mask')?.style.getPropertyValue("--icon-color")==="#FF8800","Font Awesome preview uses the chosen tint");
  fa.faIcon=undefined;fa.iconSource="image";preview();
  check(!!document.querySelector('[data-tile="0"] img') && !document.querySelector('[data-tile="0"] .icon-mask'),"Uploaded images are not tinted");
  reset({pageCount:1,macros:[{...make(0),color:"123456",textColor:"FEDCBA",tileName:"Legacy"}]}); selectTile(0);
  check(document.querySelector(".macro-fields").firstElementChild.querySelector('[data-k="actionMode"]'), "Button type is the first tile setting");
  change("name","Visible");
  check(cfg.macros[0].name==="Visible" && !document.querySelector('[data-k="tileName"]'), "Optional button label remains; tile naming removed");
  for(const theme of Object.keys(widgetThemes)) {
   change("widgetTheme",theme);
   check(document.querySelector('[data-k="color"]').disabled && cfg.macros[0].color==="123456", "Preset preserves custom color: "+theme);
   check(document.querySelector('[data-gradient-color]').value.toUpperCase()==="#"+widgetThemes[theme][0]&&document.querySelector('[data-gradient-color]').disabled, "Preset gradient shown without overwriting custom stops: "+theme);
  }
  change("widgetTheme","custom");
  check(!document.querySelector('[data-k="color"]').disabled && document.querySelector('[data-k="color"]').value==="#123456", "Custom colors restored");
  cfg.macros[0].widgetGradient="335577";render();change("widgetLayout","panel");
  check(cfg.macros[0].widgetGradient==="335577" && cfg.macros[0].widgetLayout==="panel" && !!document.querySelector('[data-tile="0"] .widget-heading'), "Modern card and custom gradient update the preview");
  change("widgetTheme","cloud");
  check(document.querySelector('[data-gradient-color]').disabled && cfg.macros[0].widgetGradient==="335577", "Preset preserves custom gradient");
  change("widgetTheme","custom");
  check(gradientStops(cfg.macros[0])[1].color==="335577", "Custom gradient restored");
  check(!document.querySelector('[data-k="actionMode"] option[value="simple_link"],[data-k="actionMode"] option[value="page_navigation"]'),"Removed actions cannot be selected for new tiles");
  const widgetConfig = structuredClone(cfg);
  change("actionMode","advanced");
  change("url","https://example.com/trigger");
  cfg.macros[0].tileGradient="AA7744";render();change("visualScale","75");
  prepareMacroAction(cfg.macros[0],0);
  check(document.querySelector('[data-tile="0"]').style.backgroundImage.includes("linear-gradient"), "Action preview shows tile gradient");
  check(cfg.macros[0].visualScale===75 && document.querySelector('[data-tile="0"] .ico').style.transform==="scale(0.75)", "Visual size persists and updates the preview");
  reset({macros:[{...emptyMacro(),actionMode:"page_navigation",pageDirection:"previous"}]});selectTile(0);
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].pageDirection==="previous" && document.querySelector('[data-k="actionMode"]').value==="page_navigation", "Existing navigation tiles are preserved");
  reset({macros:[{...emptyMacro(),url:"https://example.com/trigger"}]});selectTile(0);
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].actionMode==="advanced" && cfg.macros[0].method==="GET" && cfg.macros[0].url==="https://example.com/trigger", "Existing service links remain equivalent web requests");
  for(const decoration of [{name:"Label"},{icon:"★"},{iconImage:"icon.png"},{faIcon:"solid:star"},{sound:"click"}]){
   reset({macros:[{...emptyMacro(),...decoration}]});selectTile(0);
   check(document.querySelector('[data-k="actionMode"]').value===(decoration.sound?"advanced":"display") && !document.querySelector('[data-section="icon"]').classList.contains("hidden"),"Existing decorated or sound-only links remain editable");
  }
  reset({macros:[{...emptyMacro(),actionMode:"advanced",url:"https://example.com/existing",method:"POST",body:'{"example":true}'}]});selectTile(0);
  check(document.querySelector('[data-k="actionMode"]').value==="advanced" && document.querySelector('[data-k="url"]').value==="https://example.com/existing" && document.querySelector('[data-k="body"]').value==='{"example":true}',"Existing custom request retains its fields and values");
  reset({macros:[emptyMacro()]});selectTile(0);prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].actionMode==="simple_link" && cfg.macros[0].url==="", "Empty slots remain saveable");
  let emptySaved;
  globalThis.fetch = async (url, options) => url==="/api/config"
   ? (emptySaved=JSON.parse(options.body),new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(emptySaved?.macros[0].actionMode==="simple_link" && emptySaved.macros[0].url==="", "Saving an empty layout succeeds");
  cfg.macros[0]=emptySaved.macros[0];selectTile(0);
  check(document.querySelector('[data-k="actionMode"]').value==="" && document.querySelector('[data-mode-field="advanced"]').classList.contains("hidden"),"Saved empty tile reopens with the action prompt");
  globalThis.fetch = fetchOriginal;
  reset(widgetConfig);selectTile(0);
  for(const [theme,background] of Object.entries(themeBackgrounds)){
   const control=document.querySelector("#deviceTheme");control.value=theme;control.dispatchEvent(new Event("change"));
   check(cfg.theme===theme && document.querySelector("#background").value.toUpperCase()==="#"+background, "Device theme background: "+theme);
  }
  document.querySelector("#deviceTheme").value="midnight";document.querySelector("#deviceTheme").dispatchEvent(new Event("change"));
  change("clockFormat","12h_plain");
  check(widgetTimeText(cfg.macros[0],new Date("2026-11-21T14:30:45Z"))==="2:30", "12-hour without leading zero");
  change("clockFormat","12h_seconds");
  check(widgetTimeText(cfg.macros[0],new Date("2026-11-21T09:05:03Z"))==="9:05:03 AM"&&widgetTimeText(cfg.macros[0],new Date("2026-11-21T12:05:03Z"))==="12:05:03 PM","12-hour format preserves noon and seconds without leading zeros");
  change("clockFormat","24h");
  check(widgetTimeText(cfg.macros[0],new Date("2026-11-21T09:05:03Z"))==="09:05","24-hour format keeps its leading zero");
  change("clockFormat","12h_plain_seconds");
  check(widgetTimeText(cfg.macros[0],new Date("2026-11-21T00:30:45Z"))==="12:30:45", "Midnight without AM/PM and with seconds");
  change("widgetTextSize","40"); change("widgetGraphic","analog"); change("widget","date");
  check(cfg.macros[0].widgetGraphic==="text", "Changing widget clears incompatible graphics");
  change("widget","pc_monitor"); change("pcStat","gpu"); change("widgetGraphic","ring"); change("pcStat","gpu_temp");
  check(cfg.macros[0].widgetGraphic==="text", "Temperature is not a percentage gauge");
  change("widget","clock");
  check(document.querySelector("[data-test-action]").disabled,"Display-only widgets cannot test actions");
  change("widget","countdown");
  let countdownRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{countdownRejected=true}
  check(countdownRejected,"Countdown requires a date");
  document.querySelector("#timeOffset").value="-300";
  check(document.querySelector('[data-k="countdownAt"]').type==="date","Countdown accepts dates without times");
  change("countdownAt","2030-01-01");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].countdownAt===Date.parse("2030-01-01T05:00:00Z")/1000,"Countdown stores midnight in the selected device offset");
  check(widgetTimeText(cfg.macros[0],new Date("2029-12-31T23:59:00Z"))==="1 day" && widgetTimeText(cfg.macros[0],new Date("2030-01-02T00:00:00Z"))==="0 days","Countdown counts calendar days and stops at zero");
  change("widget","stopwatch");
  check(widgetTimeText(cfg.macros[0],new Date())==="00:00:00","Stopwatch preview starts at zero");
  change("widget","pomodoro");
  check(widgetTimeText(cfg.macros[0],new Date())==="Focus\n25:00","Pomodoro preview starts with 25 minutes of focus");
  change("focusMinutes","37");change("breakMinutes","9");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].focusMinutes===37 && cfg.macros[0].breakMinutes===9 && widgetTimeText(cfg.macros[0],new Date())==="Focus\n37:00","Custom Pomodoro durations are stored and previewed");
  change("focusMinutes","0");
  let durationRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{durationRejected=true}
  check(durationRejected,"Invalid Pomodoro duration is rejected");
  change("focusMinutes","37");
  change("widgetGraphic","custom_icon");change("icon","★");
  check(!document.querySelector('[data-k="iconSource"]').closest(".field").classList.contains("hidden") && document.querySelector('[data-tile="0"] .widget-graphic .ico')?.textContent==="★","Text widget displays the chosen icon");
  for(const widget of ["clock","date","pc_monitor","weather","air_quality","countdown","stopwatch","pomodoro"]){
   change("widget",widget);
   check(!document.querySelector("[data-widget-graphic]").classList.contains("hidden") && !document.querySelector('[data-k="iconSource"]').closest(".field").classList.contains("hidden") && document.querySelector('[data-tile="0"] .widget-graphic .ico')?.textContent==="★","Custom icon is editable and previewed for "+widget);
  }
  for(const type of ["stock","crypto"]){
   change("widget",type);
   check(!document.querySelector('[data-k="iconSource"]').closest(".field").classList.contains("hidden") && document.querySelector('[data-tile="0"] .widget-graphic .ico')?.textContent==="★","Custom icon is editable and previewed for "+type);
  }
  change("widget","pomodoro");
  check(widgetGraphicAllowed("icon","clock","summary"),"Legacy icon badges remain valid");
  cfg.macros[0].imagePreview="data:image/png;base64,AA";cfg.macros[0].iconSource="image";preview();
  check(!!document.querySelector('[data-tile="0"] .widget-graphic img'),"Widget preview displays selected image icons");
  delete cfg.macros[0].imagePreview;cfg.macros[0].iconSource="emoji";preview();
  change("font","cousine");change("textAlign","left");change("textDecoration","underline");change("textSpacing","2");
  const formatted=document.querySelector('[data-tile="0"] .widget');
  check(cfg.macros[0].font==="cousine" && formatted.style.fontFamily.includes("Consolas") && formatted.style.textAlign==="left" && formatted.style.textDecoration==="underline","Per-tile text formatting is reflected in the preview");
  check(document.querySelector('[data-k="font"]').options.length>=16,"A varied selection of bundled fonts is available");
  for(const name of ["lobster","pacifico","sharetechmono","righteous","acme","bangers","patrickhand","pressstart2p","spacemono"]){
   change("font",name);
   check(document.querySelector('[data-tile="0"] .widget').style.fontFamily.includes(previewFonts[name]),"Distinct preview font selected: "+name);
   check((await document.fonts.load(`24px "${previewFonts[name]}"`)).length===1,"Preview font loads: "+name);
   const saved=structuredClone(cfg.macros[0]);prepareMacroAction(saved,0);
   check(saved.font===name,"Font survives preparation for saving: "+name);
  }
  const fontWidth=name=>{widgetMeasure.font=`36px "${previewFonts[name]}"`;return widgetMeasure.measureText("GooPad Quick").width};
  check(fontWidth("lobster")!==fontWidth("pacifico"),"Script previews render with distinct font metrics");
  change("widgetGraphic","text");change("widget","weather");
  check(document.querySelector("[data-icon-color]").classList.contains("hidden") && !document.querySelector("[data-widget-graphic]").classList.contains("hidden") && document.querySelector('[data-k="widgetGraphic"] option[value="text"]').textContent==="Automatic Weather Icon","Default weather uses its multicolor icon without an icon color picker");
  check(!widgetGraphicSvg({widget:"weather"},new Date(),"Data unavailable"),"Unavailable weather does not show a stale graphic");
  check(widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nClear").includes("<svg"),"Weather with data shows a graphic");
  check(widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nPartly cloudy").includes("#FBBF24") && widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nPartly cloudy").includes("#CBD5E1"),"Partly cloudy weather uses distinct sun and cloud colors");
  check(!widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nCloudy").includes("#FBBF24") && widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nCloudy").includes("#64748B"),"Overcast weather uses a shaded cloud without sun");
  check(widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nSnow").includes("#F8FAFC") && widgetGraphicSvg({widget:"weather"},new Date(),"72 F\nRain").includes("#38BDF8"),"Weather precipitation uses condition colors");
  let weatherRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{weatherRejected=true}
  check(weatherRejected,"Weather requires coordinates");
  change("weatherLatitude","40.71");change("weatherLongitude","-74.01");change("temperatureUnit","F");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].weatherLatitude===40.71 && cfg.macros[0].weatherLongitude===-74.01 && cfg.macros[0].temperatureUnit==="F","Weather coordinates and units are stored");
  change("widgetGraphic","custom_icon");
  prepareMacroAction(cfg.macros[0],0);render();
  check(cfg.macros[0].widgetGraphic==="custom_icon" && !document.querySelector("[data-icon-color]").classList.contains("hidden") && document.querySelector('[data-tile="0"] .widget-graphic .ico')?.textContent==="★","Custom weather icon survives validation and editor reload");
  let weatherIconRejected=false;try{prepareMacroAction({...cfg.macros[0],iconSource:"image",iconImage:undefined},0)}catch{weatherIconRejected=true}
  check(weatherIconRejected,"Custom weather image must be uploaded before saving");
  cfg.macros[0].iconSource="image";cfg.macros[0].imagePreview="data:image/png;base64,AA";preview();
  check(!!document.querySelector('[data-tile="0"] .widget-graphic img'),"Custom weather image replaces the automatic graphic");
  cfg.macros[0].iconSource="emoji";delete cfg.macros[0].imagePreview;
  change("widgetGraphic","text");
  check(document.querySelector("[data-icon-color]").classList.contains("hidden"),"Returning to automatic weather hides custom icon colors");
  change("weatherLatitude","91");
  weatherRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{weatherRejected=true}
  check(weatherRejected,"Out-of-range weather coordinates rejected");
  change("widget","air_quality");
  check(!document.querySelector('[data-widget-option="weather"]').classList.contains("hidden"),"Air quality uses the location fields");
  let airRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{airRejected=true}
  check(airRejected,"Air quality rejects invalid coordinates");
  change("weatherLatitude","40.71");prepareMacroAction(cfg.macros[0],0);
  change("widget","stock");change("marketSymbol","aapl");
  check(cfg.macros[0].widget==="stock" && document.querySelector('[data-k="stockProvider"]').value==="finnhub" && !document.querySelector('[data-stock-provider="finnhub"]').classList.contains("hidden"),"New stock defaults to Finnhub and shows its key field");
  let stockRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{stockRejected=true}
  check(stockRejected,"Finnhub requires its own API key");
  change("finnhubKey","bad key");stockRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{stockRejected=true}
  check(stockRejected,"Finnhub rejects a key containing spaces");
  change("finnhubKey","finn-test_123");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].marketSymbol==="AAPL" && cfg.macros[0].stockProvider==="finnhub" && cfg.macros[0].finnhubKey==="finn-test_123","Finnhub symbol and key saved");
  change("stockProvider","alphavantage");
  check(document.querySelector('[data-stock-provider="finnhub"]').classList.contains("hidden") && !document.querySelector('[data-stock-provider="alphavantage"]').classList.contains("hidden"),"Switching provider changes key field");
  stockRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{stockRejected=true}
  check(stockRejected,"Alpha Vantage requires its own key");
  change("stockKey","test123");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].finnhubKey==="finn-test_123" && cfg.macros[0].stockKey==="test123","Provider switch preserves both keys");
  render();check(document.querySelector('[data-k="stockProvider"]').value==="alphavantage","Selected provider survives editor reload");
  delete cfg.macros[0].stockProvider;render();
  check(document.querySelector('[data-k="stockProvider"]').value==="alphavantage" && !document.querySelector('[data-stock-provider="alphavantage"]').classList.contains("hidden"),"Legacy stock tile retains Alpha Vantage");
  prepareMacroAction(cfg.macros[0],0);check(cfg.macros[0].stockProvider==="alphavantage","Saving legacy stock keeps Alpha Vantage");
  change("stockProvider","finnhub");prepareMacroAction(cfg.macros[0],0);render();
  check(document.querySelector('[data-k="stockProvider"]').value==="finnhub" && cfg.macros[0].stockKey==="test123" && cfg.macros[0].finnhubKey==="finn-test_123","Finnhub round trip retains both provider keys");
  change("widget","crypto");
  check(cfg.macros[0].widget==="crypto" && document.querySelector('[data-widget-option="stock"]').classList.contains("hidden"),"Market type switches to cryptocurrency and hides the stock key");
  render();check(document.querySelector('[data-k="widget"]').value==="crypto" && document.querySelector("[data-widget-category]").value==="market","Existing cryptocurrency tile opens under Market");
  change("marketSymbol","BTC&bad");
  let cryptoRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{cryptoRejected=true}
  check(cryptoRejected,"Unsafe crypto symbol rejected");
  change("marketSymbol","btc");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].marketSymbol==="BTC","Crypto symbol normalized");
  cfg.macros[0].widget="github";render();change("githubUser","bad--name");
  check(document.querySelector('[data-widget-category] option[value="legacy"]').disabled,"Existing GitHub tiles remain editable without offering them as a new choice");
  let githubRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{githubRejected=true}
  check(githubRejected,"Invalid GitHub usernames are rejected");
  change("githubUser","octocat");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].githubUser==="octocat","GitHub username is saved");
  change("widget","clock");
  cfg.companionAddress="https://192.0.2.20:8787";cfg.companionToken="x".repeat(32);cfg.companionCertificate=certificate;
  const systemMacro={...emptyMacro(),actionMode:"pc_macro",pcSteps:[{type:"system",value:"monitor_off"}]};
  preparePcMacro(systemMacro,0);
  check(JSON.parse(systemMacro.body).steps[0].value==="monitor_off", "PC monitor control is serialized");
  systemMacro.pcSteps.push({type:"wait",value:0});
  let systemRejected=false;try{preparePcMacro(systemMacro,0)}catch{systemRejected=true}
  check(systemRejected,"PC system control must be the final step");
  const home={...emptyMacro(),actionMode:"home_assistant",homeAddress:"https://home.example.com",homeEntity:"switch.lamp",homeToken:"test-token",homeOperation:"toggle"};
  prepareMacroAction(home,0);
  check(home.url==="https://home.example.com/api/services/switch/toggle" && home.method==="POST" && JSON.parse(home.body).entity_id==="switch.lamp" && home.secret==="", "Home Assistant switch creates authenticated action");
  for(const bad of [{homeAddress:"http://example.com"},{homeEntity:"switch.bad/path"},{homeToken:"bad\r\nheader"}]){
   let rejected=false;try{prepareMacroAction({...home,...bad},0)}catch{rejected=true}
   check(rejected,"Home Assistant rejects insecure or invalid inputs");
  }
  let stored;
  globalThis.fetch = async (url, options) => url==="/api/config"
   ? (stored=JSON.parse(options.body), new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  document.querySelector("#hideEmptyTiles").checked=true;
  await document.querySelector("#save").onclick();
  check(stored?.macros[0].name==="Visible" && !("tileName" in stored.macros[0]) && stored.macros[0].widgetTextSize===40 && stored.macros[0].widgetLayout==="panel" && stored.macros[0].widgetGradient==="335577" && stored.hideEmptyTiles===true, "Save preserves widget layout, gradient, sizing and hiding");
  check(document.querySelector("#backup").closest("section")!==document.querySelector("#otaGithub").closest("section") &&
        document.querySelector("#otaGithub").textContent==="Check for Update" && !document.querySelector("#otaSd"),
        "Backup and firmware are separate and SD firmware install is device-only");
  reset({macros:[make(0)]});
  const backup=structuredClone(cfg);backup.macros[0].name="Restored";
  let requests=[],savedBackup;
  globalThis.fetch=async (url,options={})=>{
   requests.push([url,options.method||"GET"]);
   if(url==="/api/backup")return new Response(JSON.stringify(backup),{status:200});
   if(url==="/api/config" && options.method==="POST"){savedBackup=JSON.parse(options.body);return new Response('{"ok":true}',{status:200})}
   if(url==="/api/config")return new Response(JSON.stringify(backup),{status:200});
   throw Error("Unexpected request "+url);
  };
  globalThis.confirm=()=>false;
  await document.querySelector("#restore").onclick();
  check(requests.length===0,"Cancel restore makes no requests");
  globalThis.confirm=()=>true;
  document.querySelector("#backgroundSample").src="data:image/png;base64,AA==";
  document.querySelector("#backgroundSample").classList.remove("hidden");
  await document.querySelector("#restore").onclick();
  check(savedBackup.macros[0].name==="Restored" && cfg.macros[0].name==="Restored" &&
        requests.map(r=>r.join(" ")).join(",")==="/api/backup GET,/api/config POST,/api/config GET" &&
        document.querySelector("#backupStatus").textContent==="Backup restored.", "Restore uses validated config save and reloads the editor");
  check(document.querySelector("#backgroundSample").classList.contains("hidden"),"Restore clears stale background preview");
  requests=[];
  globalThis.fetch=async ()=>new Response("No configuration backup on SD",{status:404});
  await document.querySelector("#restore").onclick();
  check(document.querySelector("#backupStatus").textContent.includes("No configuration backup on SD"),"Restore reports missing backup");
  requests=[];
  globalThis.fetch=async (url,options={})=>{
   requests.push([url,options.method||"GET"]);
   return new Response(url==="/api/backup"?JSON.stringify(backup):'{"ok":true,"restarting":true}',{status:200});
  };
  await document.querySelector("#restore").onclick();
  check(requests.length===2 && document.querySelector("#backupStatus").textContent.includes("restarting"),"Restore reports HomeKit restart without reloading");
  let updateRequests=[];
  globalThis.fetch=async (url,options={})=>{
   updateRequests.push([url,options.method||"GET",options.body]);
   return new Response(JSON.stringify(options.method==="POST"?{ok:true,restarting:true}:{version:"9.0.0",available:true}),{status:200});
  };
  globalThis.confirm=()=>false;
  await document.querySelector("#otaGithub").onclick();
  check(updateRequests.length===1 && document.querySelector("#firmwareStatus").textContent.includes("9.0.0"),"Update check allows cancel without installation");
  globalThis.confirm=()=>true;
  await document.querySelector("#otaGithub").onclick();
  check(updateRequests.length===3 && updateRequests[2][0]==="/api/update" &&
        JSON.parse(updateRequests[2][2]).version==="9.0.0" &&
        document.querySelector("#firmwareStatus").textContent.includes("restarting"),"Update installs the checked version through GooPad");
  globalThis.fetch=async ()=>new Response('{"version":"9.0.0","available":false}',{status:200});
  await document.querySelector("#otaGithub").onclick();
  check(document.querySelector("#firmwareStatus").textContent==="GooPad is up to date.","Update check reports current firmware");
  globalThis.fetch=async ()=>new Response("GitHub unavailable",{status:500});
  await document.querySelector("#otaGithub").onclick();
  check(document.querySelector("#firmwareStatus").textContent.includes("GitHub unavailable") && !document.querySelector("#otaGithub").disabled,"Update errors are visible and retryable");
  globalThis.fetch=async (url,options={})=>options.method==="POST"
   ? new Response("Release changed",{status:500})
   : new Response('{"version":"9.0.0","available":true}',{status:200});
  await document.querySelector("#otaGithub").onclick();
  check(document.querySelector("#firmwareStatus").textContent.includes("Release changed") && !document.querySelector("#otaGithub").disabled,"Install failures are visible and retryable");
  globalThis.confirm=confirmOriginal;globalThis.fetch=fetchOriginal;
  const graphic=document.createElement("span");
  graphic.innerHTML=widgetGraphicSvg({widget:"clock",widgetGraphic:"icon",widgetAccent:'" onload="invalid'},new Date(),"");
  check(!graphic.querySelector("svg").hasAttribute("onload"), "Legacy accent data cannot inject SVG attributes");
  pcStatsPreview={gpu:"GPU\n99.9%",ram:"RAM\n100.0%"};
  const examples=[
   {widget:"clock",widgetGraphic:"analog",clockFormat:"12h_seconds"},
   {widget:"date",widgetGraphic:"calendar",dateFormat:"iso"},
   {widget:"pc_monitor",widgetGraphic:"bar",pcStat:"ram"},
   {widget:"pc_monitor",widgetGraphic:"ring",pcStat:"gpu"}
  ];
  for(const cols of [2,3,4]) for(const rows of [2,3,4])
   for(const shape of ["rounded","square","circle","pill","soft"])
    for(const size of [0,12,14,16,20,28,40]) for(const name of ["","Label"]) {
     const themes=["custom",...Object.keys(widgetThemes)];
     cfg={...cfg,cols,rows,pageCount:1,macros:examples.map(extra=>({...emptyMacro(),...extra,actionMode:"widget",widgetTheme:themes[layouts%themes.length],widgetLayout:"panel",widgetTextSize:size,shape,name}))};
     document.querySelector("#cols").value=cols; document.querySelector("#rows").value=rows; preview();
     for(const el of document.querySelectorAll("#preview .widget")) {
      layouts++;
      const range=document.createRange(); range.selectNodeContents(el);
      const bounds=range.getBoundingClientRect(), box=el.closest("button").getBoundingClientRect();
      check(bounds.left>=box.left+1 && bounds.right<=box.right-1 && bounds.top>=box.top+1 && bounds.bottom<=box.bottom-1 && el.scrollHeight<=el.clientHeight+1,
       `Text clipped: ${cols}x${rows} ${shape} ${size} ${name} ${el.dataset.widget}`);
      const heading=el.closest("button").querySelector(".widget-heading");
      if(heading){const h=heading.getBoundingClientRect();check(h.bottom<=bounds.top+1,`Heading overlaps reading: ${cols}x${rows} ${shape} ${size} ${name} ${el.dataset.widget}`)}
      if(["circle","pill","soft"].includes(shape))check(!heading,`Curved tile should use classic layout: ${shape}`);
     }
    }
  return {checks,layouts};
 } finally {
  document.querySelector("#preview").dispatchEvent(new DragEvent("dragend",{bubbles:true}));
  globalThis.confirm=confirmOriginal; globalThis.fetch=fetchOriginal;
  [pcMonitorPreview,pcStatsPreview,pcStatsFPreview]=monitorOriginal;
  document.querySelector("#siteTheme").value=siteThemeOriginal;document.querySelector("#siteTheme").dispatchEvent(new Event("change"));
  if(savedSiteThemeOriginal===null)localStorage.removeItem("goopad-site-theme");
  else localStorage.setItem("goopad-site-theme",savedSiteThemeOriginal);
  cfg=original; selectedTile=-1; setForm();
 }
};
