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
  const primaryColors=new Set(),secondaryColors=new Set();
  for(const theme of ["dark","light","ocean","forest","violet","ember"]){
   const control=document.querySelector("#siteTheme");control.value=theme;control.dispatchEvent(new Event("change"));
   check(document.body.dataset.siteTheme===theme && document.body.classList.contains("light")===(theme==="light") && localStorage.getItem("goopad-site-theme")===theme,"Website theme persists: "+theme);
   primaryColors.add(getComputedStyle(document.querySelector("#save")).backgroundColor);
   secondaryColors.add(getComputedStyle(document.querySelector("#backup")).backgroundColor);
  }
  check(primaryColors.size===6 && secondaryColors.size===6,"Website themes recolor primary and secondary buttons");
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
  reset({pageCount:1,macros:[website],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  const websiteFields=document.querySelector('[data-mode-field="pc_open_url"]');
  check(!websiteFields.querySelector('[data-k="pcAddress"],[data-k="secret"]') && !!websiteFields.querySelector('[data-k="pcTarget"]'), "Website editor asks only for website");
  check(macroConfigured(website), "Draft website remains visible before save");
  prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.20:8787/open" && website.secret===cfg.companionToken && website.method==="POST" && JSON.parse(website.body).url===website.pcTarget, "Website uses verified HTTPS pairing and default port");
  reset({pageCount:1,macros:[emptyMacro()],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  check(!document.querySelector('[data-k="actionMode"] option[value="pc_open_url"]'),"New PC websites are configured inside the PC builder");
  change("actionMode","pc_macro");
  const starter=document.querySelector("[data-pc-preset]");starter.value="open";starter.dispatchEvent(new Event("change"));
  const websiteInput=document.querySelector('[data-step-fields] [data-value]');websiteInput.value="https://example.com/test";websiteInput.dispatchEvent(new Event("input"));
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].url==="https://192.0.2.20:8787/run" && JSON.parse(cfg.macros[0].body).steps[0].value==="https://example.com/test","PC builder sends an open-website step");
  cfg.macros[0].pcSteps[0].value="https://example.com/"+("a".repeat(180));
  let longWebsiteRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{longWebsiteRejected=true}
  check(longWebsiteRejected,"PC website steps reject links the Companion cannot accept");
  reset({pageCount:1,macros:[website],companionAddress:"192.0.2.20",companionToken:"x".repeat(32),companionCertificate:certificate});selectTile(0);
  website.pcAddress="192.0.2.30:8787";website.secret="y".repeat(32);
  prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.20:8787/open" && website.secret===cfg.companionToken, "Settings override legacy website pairing");
  cfg.companionAddress="";cfg.companionToken="";website.secret="y".repeat(32);prepareMacroAction(website,0);
  check(website.url==="https://192.0.2.30:8787/open" && website.secret==="y".repeat(32), "Legacy website address uses HTTPS with the paired certificate");
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
  check(secureSaved?.companionCertificate===certificate && secureSaved.companionAddress==="https://192.0.2.20:8787" && secureSaved.macros[0].url==="https://192.0.2.20:8787/open","Save persists certificate and canonical HTTPS macro");
  globalThis.confirm=()=>false;
  let triggered=false;
  globalThis.fetch=async(url,options)=>url==="/api/trigger"?(triggered=true,new Response("{}")):fetchOriginal(url,options);
  await document.querySelector("[data-test-action]").onclick();
  check(!triggered,"Cancel prevents real saved action execution");
  globalThis.confirm=confirmOriginal;globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[{...emptyMacro(),icon:"⚙",sound:"chime"}]});selectTile(0);
  let emojiUploads=0,emojiPayload;
  globalThis.fetch=async (url,options)=>{
   if(String(url).startsWith("/api/icon?")){emojiUploads++;emojiPayload=options.body;return new Response("{}",{status:200})}
   return fetchOriginal(url,options)
  };
  await upgradeEmojiIcons();
  const emoji=cfg.macros[0];
  check(emoji.iconSource==="emoji" && emoji.iconFormat==="rgb565a8" && emoji.iconSize===160 && !!emoji.emojiImageKey && emojiPayload instanceof Uint8Array, "Full-color emoji uses existing transparent image upload");
  const alpha=emojiPayload[0]===71&&emojiPayload[1]===80&&emojiPayload[2]===82&&emojiPayload[3]===49
   ? Array.from({length:(emojiPayload.length-6)/5},(_,i)=>emojiPayload[10+i*5])
   : Array.from(emojiPayload.slice(160*160*2));
  check(alpha.some(value=>value===0) && alpha.some(value=>value>0), "Emoji upload preserves transparency and visible glyph pixels");
  await upgradeEmojiIcons();
  check(emojiUploads===1, "Unchanged saved emoji image is reused");
  change("emojiRender","symbol");
  check(!emoji.iconImage && !emoji.imagePreview, "Switching to built-in symbol removes raster image");
  const gif={...emptyMacro(),iconSource:"gif"};cfg.macros=[gif];selectedTile=0;render();
  uploadMessage(0,"Uploading animation to SD…");
  const notice=document.querySelector('[data-visual="gif"] [data-upload-status]'),description=notice.previousElementSibling,noticeStyle=getComputedStyle(notice);
  const progressColor=noticeStyle.color;
  check(progressColor!==getComputedStyle(description).color && Number(noticeStyle.fontWeight)>=700 && noticeStyle.borderTopWidth==="1px", "GIF progress is visually distinct from its description");
  uploadMessage(0,"GIF upload failed.",true);
  check(getComputedStyle(notice).color!==progressColor, "Upload errors have a distinct color");
  globalThis.fetch=fetchOriginal;
  reset({pageCount:1,macros:[{...make(0),color:"123456",textColor:"FEDCBA",tileName:"Legacy"}]}); selectTile(0);
  change("name","Visible");
  check(cfg.macros[0].name==="Visible" && !document.querySelector('[data-k="tileName"]'), "Optional button label remains; tile naming removed");
  for(const theme of Object.keys(widgetThemes)) {
   change("widgetTheme",theme);
   check(document.querySelector('[data-k="color"]').disabled && cfg.macros[0].color==="123456", "Preset preserves custom color: "+theme);
   check(document.querySelector('[data-k="widgetGradient"]').value.toUpperCase()==="#"+widgetThemes[theme][1], "Preset gradient shown: "+theme);
  }
  change("widgetTheme","custom");
  check(!document.querySelector('[data-k="color"]').disabled && document.querySelector('[data-k="color"]').value==="#123456", "Custom colors restored");
  change("widgetGradient","#335577");change("widgetLayout","panel");
  check(cfg.macros[0].widgetGradient==="335577" && cfg.macros[0].widgetLayout==="panel" && !!document.querySelector('[data-tile="0"] .widget-heading'), "Modern card and custom gradient update the preview");
  change("widgetTheme","cloud");
  check(document.querySelector('[data-k="widgetGradient"]').disabled && cfg.macros[0].widgetGradient==="335577", "Preset preserves custom gradient");
  change("widgetTheme","custom");
  check(document.querySelector('[data-k="widgetGradient"]').value==="#335577", "Custom gradient restored");
  change("actionMode","page_navigation");
  change("pageDirection","previous");change("tileGradient","#AA7744");change("visualScale","75");
  prepareMacroAction(cfg.macros[0],0);
  check(macroConfigured(cfg.macros[0]) && cfg.macros[0].pageDirection==="previous" && cfg.macros[0].url==="", "Page navigation persists as a local action");
  check(document.querySelector("[data-test-action]").disabled && document.querySelector('[data-tile="0"]').style.backgroundImage.includes("linear-gradient"), "Navigation preview shows tile gradient and disables remote testing");
  check(cfg.macros[0].visualScale===75 && document.querySelector('[data-tile="0"] .ico').style.transform==="scale(0.75)", "Visual size persists and updates the preview");
  cfg.macros[0].pageDirection="elsewhere";
  let badDirection=false;try{prepareMacroAction(cfg.macros[0],0)}catch{badDirection=true}
  check(badDirection,"Invalid page navigation rejected");
  change("pageDirection","next");change("actionMode","widget");
  for(const [theme,background] of Object.entries(themeBackgrounds)){
   const control=document.querySelector("#deviceTheme");control.value=theme;control.dispatchEvent(new Event("change"));
   check(cfg.theme===theme && document.querySelector("#background").value.toUpperCase()==="#"+background, "Device theme background: "+theme);
  }
  document.querySelector("#deviceTheme").value="midnight";document.querySelector("#deviceTheme").dispatchEvent(new Event("change"));
  change("clockFormat","12h_plain");
  check(widgetTimeText(cfg.macros[0],new Date("2026-11-21T14:30:45Z"))==="02:30", "12-hour without AM/PM");
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
  change("countdownAt","2030-01-01T12:30");
  prepareMacroAction(cfg.macros[0],0);
  check(Number.isInteger(cfg.macros[0].countdownAt) && widgetTimeText({widget:"countdown",countdownAt:1577836800},new Date()).startsWith("00:00:00"),"Countdown stores absolute seconds and stops at zero");
  change("widget","stopwatch");
  check(widgetTimeText(cfg.macros[0],new Date())==="00:00:00","Stopwatch preview starts at zero");
  change("widget","weather");
  let weatherRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{weatherRejected=true}
  check(weatherRejected,"Weather requires coordinates");
  change("weatherLatitude","40.71");change("weatherLongitude","-74.01");change("temperatureUnit","F");
  prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].weatherLatitude===40.71 && cfg.macros[0].weatherLongitude===-74.01 && cfg.macros[0].temperatureUnit==="F","Weather coordinates and units are stored");
  change("weatherLatitude","91");
  weatherRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{weatherRejected=true}
  check(weatherRejected,"Out-of-range weather coordinates rejected");
  change("widget","stock");change("marketSymbol","aapl");
  let stockRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{stockRejected=true}
  check(stockRejected,"Stock requires an API key");
  change("stockKey","test123");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].marketSymbol==="AAPL" && cfg.macros[0].stockKey==="test123","Stock symbol normalized and key saved");
  change("widget","crypto");change("marketSymbol","BTC&bad");
  let cryptoRejected=false;try{prepareMacroAction(cfg.macros[0],0)}catch{cryptoRejected=true}
  check(cryptoRejected,"Unsafe crypto symbol rejected");
  change("marketSymbol","btc");prepareMacroAction(cfg.macros[0],0);
  check(cfg.macros[0].marketSymbol==="BTC","Crypto symbol normalized");
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
