globalThis.runGooPadEditorTests = async function () {
 if (location.protocol !== "file:") throw Error("Run only against the local editor, never a device.");
 const original = structuredClone(cfg), confirmOriginal = globalThis.confirm, fetchOriginal = globalThis.fetch;
 const monitorOriginal = [pcMonitorPreview, pcStatsPreview, pcStatsFPreview];
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
  for (const page of [0,1,2]) {
   reset({}); scrollPreviewPage(page); document.querySelector("#addPage").click();
   check(cfg.pageCount === 4 && currentPreviewPage() === page+1, "Insertion/selection after current page");
   check(cfg.macros[(page+1)*6].name === "", "Inserted page starts empty");
   if (page<2) check(cfg.macros[(page+2)*6].name === "W"+((page+1)*6), "Insertion preserves later slots");
   document.querySelector("#removePage").click();
   check(cfg.pageCount === 3 && cfg.macros[6].name === "W6", "Removing an empty inserted page");
  }
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
  check(document.querySelector("#companionAddress").value==="https://192.0.2.20:8787" && document.querySelector("#companionCertificate").value===certificate && document.querySelector("#companionPairing").value==="", "Secure bundle populates settings and clears private pasted code");
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
  }
  change("widgetTheme","custom");
  check(!document.querySelector('[data-k="color"]').disabled && document.querySelector('[data-k="color"]').value==="#123456", "Custom colors restored");
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
  let stored;
  globalThis.fetch = async (url, options) => url==="/api/config"
   ? (stored=JSON.parse(options.body), new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  document.querySelector("#hideEmptyTiles").checked=true;
  await document.querySelector("#save").onclick();
  check(stored?.macros[0].name==="Visible" && !("tileName" in stored.macros[0]) && stored.macros[0].widgetTextSize===40 && stored.hideEmptyTiles===true, "Save omits legacy names and stores sizing and hiding");
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
     cfg={...cfg,cols,rows,pageCount:1,macros:examples.map(extra=>({...emptyMacro(),...extra,actionMode:"widget",widgetTheme:["custom",...Object.keys(widgetThemes)][layouts%8],widgetTextSize:size,shape,name}))};
     document.querySelector("#cols").value=cols; document.querySelector("#rows").value=rows; preview();
     for(const el of document.querySelectorAll("#preview .widget")) {
      layouts++;
      const range=document.createRange(); range.selectNodeContents(el);
      const bounds=range.getBoundingClientRect(), box=el.closest("button").getBoundingClientRect();
      check(bounds.left>=box.left+1 && bounds.right<=box.right-1 && bounds.top>=box.top+1 && bounds.bottom<=box.bottom-1 && el.scrollHeight<=el.clientHeight+1,
       `Text clipped: ${cols}x${rows} ${shape} ${size} ${name} ${el.dataset.widget}`);
     }
    }
  return {checks,layouts};
 } finally {
  document.querySelector("#preview").dispatchEvent(new DragEvent("dragend",{bubbles:true}));
  globalThis.confirm=confirmOriginal; globalThis.fetch=fetchOriginal;
  [pcMonitorPreview,pcStatsPreview,pcStatsFPreview]=monitorOriginal;
  cfg=original; selectedTile=-1; setForm();
 }
};
