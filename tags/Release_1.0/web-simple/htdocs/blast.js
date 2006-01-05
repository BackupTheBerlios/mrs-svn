// Functions to make the batch blast interface work

var
	gNuclDbs, gProtDbs;

function initBlastForm()
{
	var v = getCookie('input');

	if (v)
		selectInput(v);

	v = getCookie('db');

	if (v)
		selectDbs(v);
	
	// auto-expand the db tree to show what was selected
	var n;
	if (document.f.db.value == "protein")
		n = gProtDbs;
	else if (document.f.db.value == "nucleotide")
		n = gNuclDbs;
	
	if (n != null)
		n.auto_expand();
	
	// restore the gapped setting
	selectGapped();
}

function updateOptions()
{
	var input_kind = document.f.input.value;
	var db_kind = document.f.db.value;
	
	if (input_kind == 'nucleotide' && db_kind == 'nucleotide')
	{
		document.getElementById("bo_via_protein").style.display = '';
		
		if (document.getElementById("via_protein").checked)
			document.getElementById("bo_matrix").style.display = '';
		else
			document.getElementById("bo_matrix").style.display = 'none';
		
		if (document.f.wordsize.options.length != 13)
		{
			document.f.wordsize.options.length = 1;
			document.f.wordsize.options[1] = new Option('7','7');
			document.f.wordsize.options[2] = new Option('8','8');
			document.f.wordsize.options[3] = new Option('9','9');
			document.f.wordsize.options[4] = new Option('10','10');
			document.f.wordsize.options[5] = new Option('11','11');
			document.f.wordsize.options[6] = new Option('12','12');
			document.f.wordsize.options[7] = new Option('13','13');
			document.f.wordsize.options[8] = new Option('14','14');
			document.f.wordsize.options[9] = new Option('15','15');
			document.f.wordsize.options[10] = new Option('16','16');
			document.f.wordsize.options[11] = new Option('17','17');
			document.f.wordsize.options[12] = new Option('18','18');
		}
	}
	else
	{
		document.getElementById("bo_matrix").style.display = '';
		document.getElementById("bo_via_protein").style.display = 'none';

		if (document.f.wordsize.options.length != 3)
		{
			document.f.wordsize.options.length = 1;
			document.f.wordsize.options[1] = new Option('2','2');
			document.f.wordsize.options[2] = new Option('3','3');
		}
	}
}

function selectInput(kind)
{
	var p = document.getElementById("prot");
	var n = document.getElementById("nucl");
	var l = document.getElementById("label1");
	
	if (kind == "protein")
	{
		p.className = 'selected';
		n.className = 'unselected';
		l.innerHTML = "Enter one or more protein sequences in FastA format";
		
		document.f.input.value = kind;
	}
	else if (kind == "nucleotide")
	{
		p.className = 'unselected';
		n.className = 'selected';
		l.innerHTML = "Enter one or more nucleotide sequences in FastA format";

		document.f.input.value = kind;
	}
	else
		alert("kan niet"); 

	setCookie("input", kind);
	updateOptions();
}

function selectDbs(kind)
{
	var p = document.getElementById("prot_d");
	var n = document.getElementById("nucl_d");
	var pd = document.getElementById("prot_dbs");
	var nd = document.getElementById("nucl_dbs");
	
	if (kind == "protein")
	{
		p.className = 'selected';
		n.className = 'unselected';
		pd.style.display = '';
		nd.style.display = 'none';

		document.f.db.value = kind;
		
		gProtDbs.auto_expand();
	}
	else if (kind == "nucleotide")
	{
		p.className = 'unselected';
		n.className = 'selected';
		pd.style.display = 'none';
		nd.style.display = '';

		document.f.db.value = kind;

		gNuclDbs.auto_expand();
	}
	else
		alert("kan niet"); 

	setCookie("db", kind);
	updateOptions();
}

function selectGapped()
{
	var g = document.getElementById('bo_gapped_cb');
	
	if (g.checked)
	{
		setCookie('gapped', true);
		
		var cb = document.getElementById('bo_gapopen');
		if (cb)
			cb.style.display = '';

		cb = document.getElementById('bo_gapextend');
		if (cb)
			cb.style.display = '';
	}
	else
	{
		setCookie('gapped', false);

		var cb = document.getElementById('bo_gapopen');
		if (cb)
			cb.style.display = 'none';

		cb = document.getElementById('bo_gapextend');
		if (cb)
			cb.style.display = 'none';
	}
}

function selectParameters(p)
{
	var tr = document.getElementById('opt_reg_tab');
	var ta = document.getElementById('opt_adv_tab');
	var br = document.getElementById('opt_reg_box');
	var ba = document.getElementById('opt_adv_box');

	if (p == 'adv') {
		tr.className = 'unselected';
		br.style.display = 'none';
		ta.className = 'selected';
		ba.style.display = '';
	}
	else {
		tr.className = 'selected';
		br.style.display = '';
		ta.className = 'unselected';
		ba.style.display = 'none';
	}
}

function doSubmit()
{
	var result = false;
	var program = "";
	
	if (document.f.input.value == "protein")
	{
		if (document.f.db.value == "protein")
			program = "blastp";
		else if (document.f.db.value == "nucleotide")
			program = "tblastn";
	}
	else if (document.f.input.value == "nucleotide")
	{
		if (document.f.db.value == "protein")
			program = "blastx";
		else if (document.f.db.value == "nucleotide")
		{
			if (document.getElementById("via_protein").checked)
				program = "tblastx";
			else
				program = "blastn";
		}
	}
	
	if (program != "")
	{
		document.f.program.value = program;
		result = true;
	}
	else
		alert("Error determining program");

	var dbs = "";
	var n;
	if (document.f.db.value == "protein")
		n = gProtDbs;
	else if (document.f.db.value == "nucleotide")
		n = gNuclDbs;
	
	while (n != null)
	{
		// only add names of leaf nodes
		if (n.isChecked() && n.children.length == 0)
		{
			if (dbs.length > 0)
				dbs += ",";
			dbs += n.dbName;
		}
		n = n.next;
	}
	
	if (dbs.length == 0)
	{
		result = false;
		alert("No databases selected");
	}
	else
		document.f.dbs.value = dbs;
	
	return result;
}

function tNode(dbName, parent, kind)
{
	this.isCollapsed = true;
	this.dbName = dbName;
	this.objName = "t_"+ dbName;
	this.children = [];
	if (parent != null)
		parent.add(this);
	if (kind == "protein")
	{
		this.next = gProtDbs;
		gProtDbs = this;
	}
	else if (kind == "nucleotide")
	{
		this.next = gNuclDbs;
		gNuclDbs = this;
	}
}

tNode.prototype.add = function(n)
{
	this.children.push(n);
	n.parent = this;
}

tNode.prototype.collapse = function()
{
	var a = [], n;
	
	for (n = 0; n < this.children.length; ++n)
		a.push(this.children[n]);

	while (a.length > 0)
	{
		var c = a.pop();
		
		var node = document.getElementById(c.objName);
		node.style.display = 'none';
		
		for (n = 0; n < c.children.length; ++n)
			a.push(c.children[n]);
	}

	this.isCollapsed = true;
}

tNode.prototype.expand = function()
{
	var a = [], n;
	
	for (n = 0; n < this.children.length; ++n)
		a.push(this.children[n]);

	while (a.length > 0)
	{
		var c = a.pop();
		
		var node = document.getElementById(c.objName);
		node.style.display = '';
		
		if (c.isCollapsed == false)
		{
			for (n = 0; n < c.children.length; ++n)
				a.push(c.children[n]);
		}
	}

	this.isCollapsed = false;
}

tNode.prototype.toggle = function()
{
	if (this.isCollapsed)
		this.expand();
	else
		this.collapse();
}

tNode.prototype.check = function(v)
{
	var a = [], n, cb;
	
	for (n = 0; n < this.children.length; ++n)
		a.push(this.children[n]);

	while (a.length > 0)
	{
		var c = a.pop();
		
		cb = document.getElementById(c.objName + "_cb");
		cb.checked = v;
		setCookie(c.objName + "_cb", v);
		
		for (n = 0; n < c.children.length; ++n)
			a.push(c.children[n]);
	}
	
	if (v == false)
	{
		var p = this.parent;
		while (p != null)
		{
			if (p.isChecked())
			{
				cb = document.getElementById(p.objName + "_cb");
				cb.checked = false;
			}
			
			p = p.parent;
		}
	}
}

tNode.prototype.isChecked = function(v)
{
	cb = document.getElementById(this.objName + "_cb");
	return cb.checked;
}

tNode.prototype.toggleSelect = function()
{
	var cb = document.getElementById(this.objName + "_cb");
	if (cb.checked)
		this.check(true);
	else
		this.check(false);
	setCookie(this.objName + "_cb", cb.checked);
}

tNode.prototype.auto_expand = function()
{
	var a = [], c, n;
	
	for (c = this; c != null; c = c.next)
		a.push(c);

	while (a.length > 0)
	{
		c = a.pop();

		if (c.isChecked())
		{
			var p = c.parent;
			
			while (p != null)
			{
				if (p.isChecked())
					p.collapse();
				else
					p.expand();
				p = p.parent;
			}
		}
		else
		{
			for (n = 0; n < c.children.length; ++n)
				a.push(c.children[n]);
		}
	}
}

// If Push and pop is not implemented by the browser
if (!Array.prototype.push)
{
	Array.prototype.push = function array_push()
	{
		for(var i=0;i<arguments.length;i++)
			this[this.length]=arguments[i];
		return this.length;
	}
};

if (!Array.prototype.pop)
{
	Array.prototype.pop = function array_pop()
	{
		lastElement = this[this.length-1];
		this.length = Math.max(this.length - 1, 0);
		return lastElement;
	}
};

// [Cookie] Sets value in a cookie
function setCookie(cookieName, cookieValue)
{
	document.cookie =
		escape(cookieName) + '=' + escape(cookieValue) + 
		"; path=/";
};

function getCookie(name)
{
    var dc = document.cookie;
    var prefix = name + "=";
    var begin = dc.indexOf("; " + prefix);

    if (begin == -1)
    {
        begin = dc.indexOf(prefix);
        if (begin != 0)
        	return null;
    }
    else
    {
        begin += 2;
    }

    var end = document.cookie.indexOf(";", begin);
    if (end == -1)
    {
        end = dc.length;
    }

    return unescape(dc.substring(begin + prefix.length, end));
}

function doRun()
{
	
}

// code to do a live update of the count as a result of the query string

var req;

function loadXMLDoc(url) 
{
    // branch for native XMLHttpRequest object
    if (window.XMLHttpRequest)
    {
        req = new XMLHttpRequest();
        req.onreadystatechange = processReqChange;
        req.open("GET", url, true);
        req.send(null);
    // branch for IE/Windows ActiveX version
    }
    else if (window.ActiveXObject)
    {
        req = new ActiveXObject("Microsoft.XMLHTTP");
        if (req)
        {
            req.onreadystatechange = processReqChange;
            req.open("GET", url, true);
            req.send();
        }
    }
}

function processReqChange() 
{
    // only if req shows "complete"
    if (req.readyState == 4)
    {
        // only if "OK"
        if (req.status == 200)
        {
			var countPanel = document.getElementById("query_count");
			
			var r = req.responseXML;
			var rr = r.getElementsByTagName('result');
			var cnt = rr[0].firstChild.data;

			countPanel.innerHTML = 'This query returns ' + cnt + ' hits';
			
			countPanel.style.display = '';
        }
//        else
//        {
//            alert("There was a problem retrieving 
//               the XML data:\n" + req.statusText);
//        }
    }
}

function updateQueryCount()
{
	var db = document.f.dbs;
	var q = document.f.mrs_query;
	
	if (q.value.length > 1)
	{
		loadXMLDoc("mrs-count.cgi?db=" + db.options[db.selectedIndex].value + "&q=" + q.value + "&w=1");
	}
	else
	{
		var countPanel = document.getElementById("query_count");
		countPanel.style.display = 'none';
	}
}
