globalThis.runGooPadEditorTests = async function () {
 if (location.protocol !== "file:") throw Error("Run only against the local editor, never a device.");
 const original = structuredClone(cfg), confirmOriginal = globalThis.confirm, fetchOriginal = globalThis.fetch;
 const monitorOriginal = [pcMonitorPreview, pcStatsPreview, pcStatsFPreview];
 let checks = 0, layouts = 0;
 const check = (value, message) => { checks++; if (!value) throw Error(message) };
 const make = i => ({...emptyMacro(), name:"W"+i, actionMode:"widget", widget:"clock"});
 const reset = settings => {
  cfg = {...original, cols:3, rows:2, pageCount:3, macros:Array.from({length:18}, (_,i)=>make(i)), ...settings};
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
  let stored;
  globalThis.fetch = async (url, options) => url==="/api/config"
   ? (stored=JSON.parse(options.body), new Response("{}",{status:200,headers:{"Content-Type":"application/json"}}))
   : fetchOriginal(url,options);
  await document.querySelector("#save").onclick();
  check(stored?.macros[0].name==="Visible" && !("tileName" in stored.macros[0]) && stored.macros[0].widgetTextSize===40, "Save omits legacy names and stores numeric sizing");
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
