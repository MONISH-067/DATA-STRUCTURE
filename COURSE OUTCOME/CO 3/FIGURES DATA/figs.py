import subprocess, re, os, html
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from PIL import Image
F="Liberation Sans"
def parse(path):
    figs={}; cur=None
    for line in open(path):
        line=line.rstrip("\n")
        if line.startswith("@FIG"): cur=line.split()[1]; figs[cur]={"nodes":{}, "root":-1}; continue
        if line.startswith("ROOT"): figs[cur]["root"]=int(line.split()[1]); continue
        t=line.split()
        if t[0]=="N": figs[cur]["nodes"][int(t[1])]=dict(key=t[2],aux=t[3],l=int(t[4]),r=int(t[5]))
        if t[0]=="B":
            i=t.index("|"); figs[cur]["nodes"][int(t[1])]=dict(keys=t[2:i],ch=[int(x) for x in t[i+1:]])
    return figs
def dot_binary(fig, color=False, hcap=False):
    out=[]; nodes=fig["nodes"]
    if fig["root"]<0: return 'digraph G{ e [label="(empty tree)",shape=plaintext]; }'
    for i,n in nodes.items():
        lab=n["key"]; fill="#DCE9F7"; fc="black"
        if color: fill="#E06666" if n["aux"]=="R" else "#333333"; fc="white"
        if hcap and n["aux"].startswith("h="): lab=f'{n["key"]}'
        out.append(f'n{i} [label="{lab}", shape=circle, style=filled, fillcolor="{fill}", fontcolor="{fc}", fontname="{F}", fontsize=11, width=0.4, fixedsize=true];')
        for side,c in (("l",n["l"]),("r",n["r"])):
            if c>=0: out.append(f'n{i} -> n{c} [arrowhead=none];')
            else: out.append(f'z{i}{side} [shape=point,width=0.01,style=invis]; n{i} -> z{i}{side} [style=invis];')
    return "digraph G{ node[fontname=\"%s\"]; nodesep=0.15; ranksep=0.3; pad=0.05; %s }"%(F,"\n".join(out))
def dot_btree(fig):
    out=[]; nodes=fig["nodes"]
    for i,n in nodes.items():
        cells="".join(f'<TD>{k}</TD>' for k in n["keys"])
        out.append(f'n{i} [shape=none, label=<<TABLE BORDER="1" CELLBORDER="1" CELLSPACING="0" CELLPADDING="4" BGCOLOR="#DCE9F7"><TR>{cells}</TR></TABLE>>, fontname="{F}", fontsize=11];')
        for c in n["ch"]: out.append(f'n{i} -> n{c} [arrowhead=none];')
    return 'digraph G{ nodesep=0.2; ranksep=0.3; pad=0.05; %s }'%("\n".join(out))
def render(dot, path, dpi=150):
    open(path+".dot","w").write(dot); subprocess.run(["dot","-Tpng",f"-Gdpi={dpi}",path+".dot","-o",path],check=True)
def nested(t, color=False, prefix="x"):
    """t = (key, left, right) or (key,left,right,'R'/'B'); None = empty"""
    out=[]; cnt=[0]
    def rec(n):
        if n is None: return None
        i=cnt[0]; cnt[0]+=1; key=n[0]; l=rec(n[1]); r=rec(n[2]); col=n[3] if len(n)>3 else None
        fill="#DCE9F7"; fc="black"
        if col: fill="#E06666" if col=="R" else "#333333"; fc="white"
        out.append(f'n{i} [label="{key}", shape=circle, style=filled, fillcolor="{fill}", fontcolor="{fc}", fontname="{F}", fontsize=11, width=0.4, fixedsize=true];')
        for side,c in (("l",l),("r",r)):
            if c is not None: out.append(f'n{i} -> n{c} [arrowhead=none];')
            else: out.append(f'z{i}{side} [shape=point,width=0.01,style=invis]; n{i} -> z{i}{side} [style=invis];')
        return i
    rec(t); return 'digraph G{ nodesep=0.15; ranksep=0.3; pad=0.05; %s }'%("\n".join(out))
def montage(items, titles, path, ncols, minw=0, scale=1.0, figw=7.0, rowh=None):
    ims=[Image.open(f) for f in items]; n=len(ims); nrows=(n+ncols-1)//ncols
    cw=max(minw,max(i.size[0] for i in ims)); ch=max(i.size[1] for i in ims)
    W=cw*ncols; H=(ch+45)*nrows
    sheet=Image.new("RGB",(W,H),"white")
    from PIL import ImageDraw, ImageFont
    d=ImageDraw.Draw(sheet)
    try: font=ImageFont.truetype("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",22)
    except: font=None
    for k,(im,t) in enumerate(zip(ims,titles)):
        r,c=divmod(k,ncols); x=c*cw+(cw-im.size[0])//2; y=r*(ch+45)+40+(ch-im.size[1])//2
        sheet.paste(im,(x,y)); tw=d.textlength(t,font=font); d.text((c*cw+(cw-tw)/2, r*(ch+45)+8),t,fill="black",font=font)
    sheet.save(path)
os.makedirs("fig",exist_ok=True)
def tmp(n): return f"fig/_{n}.png"
# ---- predict 1: array tree
render(nested(("A",("B",("D",None,None),("E",None,None)),("C",None,("F",None,None)))),"fig/arr_tree.png")
# ---- predict 2: three deletions
t0=(50,(30,(20,None,None),(40,None,None)),(70,(60,None,(65,None,None)),(80,None,None)))
t1=(50,(30,None,(40,None,None)),(70,(60,None,(65,None,None)),(80,None,None)))
t2=(50,(40,None,None),(70,(60,None,(65,None,None)),(80,None,None)))
t3=(60,(40,None,None),(70,(65,None,None),(80,None,None)))
its=[]
for i,t in enumerate((t0,t1,t2,t3)): render(nested(t),tmp(f"p2_{i}")); its.append(tmp(f"p2_{i}"))
montage(its,["inserted tree","after delete 20 (leaf)","after delete 30 (one child)","after delete 50 (two children)"],"fig/predict2.png",4)
# ---- Q1 figures
f1=parse("fig/fig_q1.txt"); its=[]; ts=[]
for k,t in (("demo","demo tree (height 3)"),("after800","after delete 800 (leaf)"),("del200","delete 200 (leaf)"),("del700","delete 700 (one child)"),("del500","delete 500 (two children)")):
    render(dot_binary(f1[k]),tmp("q1"+k)); its.append(tmp("q1"+k)); ts.append(t)
montage(its,ts,"fig/q1_trees.png",5)
# sorted vs random shape (8 keys)
render(nested((10,None,(20,None,(30,None,(40,None,(50,None,(60,None,(70,None,(80,None,None))))))))),"fig/q1_sorted8.png")
# ---- Q2: after each insertion
f2=parse("fig/fig_q2.txt"); its=[]; ts=[]
for k in ("ins10","ins20","ins30","ins40","ins50","ins25"):
    render(dot_binary(f2[k]),tmp("q2"+k)); its.append(tmp("q2"+k)); ts.append("after insert "+k[3:])
montage(its,ts,"fig/q2_steps.png",6)
# before/after rotations (manual)
rots=[("insert 30: RR at 10", (10,None,(20,None,(30,None,None))), "left rotation at 10", (20,(10,None,None),(30,None,None))),
      ("insert 50: RR at 30", (20,(10,None,None),(30,None,(40,None,(50,None,None)))), "left rotation at 30", (20,(10,None,None),(40,(30,None,None),(50,None,None)))),
      ("insert 25: RL at 20", (20,(10,None,None),(40,(30,(25,None,None),None),(50,None,None))), "right at 40, left at 20", (30,(20,(10,None,None),(25,None,None)),(40,None,(50,None,None))))]
its=[]; ts=[]
for i,(a,ta,b,tb) in enumerate(rots):
    render(nested(ta),tmp(f"r{i}a")); render(nested(tb),tmp(f"r{i}b")); its+= [tmp(f"r{i}a"),tmp(f"r{i}b")]; ts+=["before: "+a,"after: "+b]
montage(its,ts,"fig/q2_rotations.png",2,minw=520)
# ---- Q3 B-tree steps
f3=parse("fig/fig_q3.txt"); its=[]; ts=[]
for k in ("bt10","bt20","bt5","bt6","bt12","bt30","bt7","bt17"):
    render(dot_btree(f3[k]),tmp("q3"+k)); its.append(tmp("q3"+k)); ts.append("after insert "+k[2:]+(" (split)" if k in("bt6","bt17") else ""))
montage(its,ts,"fig/q3_steps.png",4)
# ---- Q4 RB steps
f4=parse("fig/fig_q4.txt"); its=[]; ts=[]
for k,t in (("rb10","insert 10"),("rb20","insert 20"),("rb30","insert 30 (rotate)"),("rb15","insert 15 (recolour)"),("rb25","insert 25"),("rb5","insert 5")):
    render(dot_binary(f4[k],color=True),tmp("q4"+k)); its.append(tmp("q4"+k)); ts.append(t)
montage(its,ts,"fig/q4_steps.png",3)
# ---- Q5 splay figures
f5=parse("fig/fig_q5.txt"); its=[]; ts=[]
for k,t in (("start","given tree"),("acc3","after access 3"),("acc9","after access 9")):
    render(dot_binary(f5[k]),tmp("q5"+k)); its.append(tmp("q5"+k)); ts.append(t)
montage(its,ts,"fig/q5_trace.png",3)
# generic zig, zig-zig, zig-zag diagrams
def gen(label_before, tb, label_after, ta, n):
    def d(t):
        out=[]; 
        def rec(n_):
            if n_ is None: return
            name,l,r=n_
            shape="circle" if name in "xpg" else "triangle"; fill="#DCE9F7" if name in "xpg" else "#EEEEEE"
            out.append(f'{name} [label="{name}", shape={shape}, style=filled, fillcolor="{fill}", fontname="{F}", width=0.4, fixedsize=true];')
            for side,c in ((0,l),(1,r)):
                if c is not None: out.append(f'{name} -> {c[0]} [arrowhead=none];'); rec(c)
                else: out.append(f'z{name}{side} [shape=point,width=0.01,style=invis]; {name} -> z{name}{side} [style=invis];')
        rec(t); return 'digraph G{ nodesep=0.2; ranksep=0.3; pad=0.05; %s }'%("\n".join(out))
    render(d(tb),tmp(n+"a")); render(d(ta),tmp(n+"b")); return [tmp(n+"a"),tmp(n+"b")],[label_before,label_after]
zig=gen("zig: before (x child of root p)",("p",("x",("A",None,None),("B",None,None)),("C",None,None)),"after",("x",("A",None,None),("p",("B",None,None),("C",None,None))),"zg1")
zz=gen("zig-zig: before (x, p, g on one side)",("g",("p",("x",("A",None,None),("B",None,None)),("C",None,None)),("D",None,None)),"after",("x",("A",None,None),("p",("B",None,None),("g",("C",None,None),("D",None,None)))),"zg2")
zg=gen("zig-zag: before (x between p and g)",("g",("p",("A",None,None),("x",("B",None,None),("C",None,None))),("D",None,None)),"after",("x",("p",("A",None,None),("B",None,None)),("g",("C",None,None),("D",None,None))),"zg3")
montage(zig[0]+zz[0]+zg[0],zig[1]+zz[1]+zg[1],"fig/q5_steps.png",2,minw=420)
# ---- charts
plt.rcParams.update({"font.family":"Liberation Sans","font.size":9})
fig,ax=plt.subplots(1,2,figsize=(7.6,2.8))
names=["BST\n(random order)","AVL","B-tree\n(order 100)"]; avg=[28.79,21.33,3.99]; worst=[52,23,4]
x=range(3); ax[0].bar([i-0.2 for i in x],avg,0.4,label="average",color="#5B84B1"); ax[0].bar([i+0.2 for i in x],worst,0.4,label="worst seen",color="#1F3A5F")
ax[0].set_xticks(list(x)); ax[0].set_xticklabels(names); ax[0].set_ylabel("node visits = disk reads"); ax[0].set_title("5,000,000 keys: reads per search",fontsize=10); ax[0].legend(frameon=False)
for i,v in enumerate(avg): ax[0].text(i-0.2,v+0.8,f"{v:.1f}",ha="center",fontsize=8)
ax[1].bar(names,[a*10 for a in avg],color=["#C0504D","#E3A21A","#3C8D5A"]); ax[1].set_ylabel("ms per search at 10 ms / read"); ax[1].set_title("Average search time",fontsize=10)
for i,v in enumerate(avg): ax[1].text(i,v*10+6,f"{v*10:.0f} ms",ha="center",fontsize=8)
for a in ax: a.spines[["top","right"]].set_visible(False)
plt.tight_layout(); plt.savefig("fig/q3_chart.png",dpi=200); plt.close()
fig,ax=plt.subplots(1,3,figsize=(7.8,2.7))
lab=["80/20\nn=1,000","uniform\nn=1,000","90% to 10\nn=1,000","80/20\nn=10,000","uniform\nn=10,000"]
sp=[8.12,12.47,4.68,12.86,17.24]; av=[7.40,9.22,5.90,10.31,12.55]
ax[0].bar([i-0.2 for i in range(5)],sp,0.4,label="splay",color="#C0504D"); ax[0].bar([i+0.2 for i in range(5)],av,0.4,label="AVL",color="#3C8D5A"); ax[0].set_xticks(range(5)); ax[0].set_xticklabels(lab,fontsize=6.5); ax[0].set_ylabel("average nodes visited per access"); ax[0].legend(frameon=False,fontsize=8); ax[0].set_title("Splay vs AVL",fontsize=10)
ax[1].bar(["RB","AVL"],[16,16],color=["#C0504D","#3C8D5A"]); ax[1].set_title("Height, 10,000 random keys",fontsize=10)
ax[2].bar(["RB","AVL"],[24,14],color=["#C0504D","#3C8D5A"]); ax[2].set_title("Height, keys 1..10,000 in order",fontsize=10)
for a in ax: a.spines[["top","right"]].set_visible(False)
plt.tight_layout(); plt.savefig("fig/q45_chart.png",dpi=200); plt.close()
fig,ax=plt.subplots(1,2,figsize=(7.6,2.9))
st=["sorted array","hash table","compact trie","AVL of strings"]; mem=[3.64,6.24,9.40,8.44]
ax[0].barh(st[::-1],mem[::-1],color=["#C0504D","#E3A21A","#5B84B1","#3C8D5A"][::-1]); ax[0].set_xlabel("MB for 200,000 words"); ax[0].set_title("Memory (measured)",fontsize=10)
for i,v in enumerate(mem[::-1]): ax[0].text(v+0.1,i,f"{v:.1f}",va="center",fontsize=8)
ops=["exact","autocomplete\n(10 words)","range\n(50 words)"]; tr=[460,466,4782]; ar=[301,307,718]; hs=[53,None,None]
x=range(3); ax[1].bar([i-0.2 for i in x],ar,0.4,label="sorted array",color="#3C8D5A"); ax[1].bar([i+0.2 for i in x],tr,0.4,label="compact trie",color="#5B84B1"); ax[1].set_yscale("log"); ax[1].set_xticks(list(x)); ax[1].set_xticklabels(ops,fontsize=8); ax[1].set_ylabel("median ns per operation (log)"); ax[1].legend(frameon=False,fontsize=8); ax[1].set_title("Response time, 200,000 words",fontsize=10)
for a in ax: a.spines[["top","right"]].set_visible(False)
plt.tight_layout(); plt.savefig("fig/q7_chart.png",dpi=200); plt.close()
print("figs ok")
