function doSearch()
{
	document.f.first.value = 0;

	try {
		document.f.format.value = "summary";
	}
	catch (e) {
	}

	document.f.submit();
}

function doSave(url, db, id)
{
	var e;

	if (db) {
		url += "?db=" + db;
	}
	else {
		e = document.f["db"];
		url += "?db=" + e.options[e.selectedIndex].value;
	}

	e = document.getElementById('list');
	var selected = false;

	if (e) {
		var cb = e.getElementsByTagName('input');
		
		for (var i = 0; i < cb.length; ++i) {
			if (cb[i].checked) {
				url += "&id=" + cb[i].id;
				selected = true;
			}
		}
	}
	
	if (selected == false && document.f) {
		if (id) {
			url += "&id=" + id;
		}
		else {
			e = document.f["query"];
			url += "&query=" + escape(e.value);
		}
	}
	
	url += "&exp=1";
	
	if (document.f) {
		e = document.f["format"];
		url += "&format=" + e.options[e.selectedIndex].value;
		
		e = document.f["save_to"];
		url += "&save_to=" + e.options[e.selectedIndex].value;
	}

		// if the users wants to display the results in a (new) window
	if (e.selectedIndex == 0) {
		var newWindow = window.open(url);
	}
	else {
		// otherwise avoid opening an empty, unused window
		window.location = url;
	}
}

function doPage(first, query)
{
	document.f.first.value = first;
	document.f.query.value = query;
	document.f.submit();
}

function doClear()
{
	document.f.query.value = "";
}

function doReloadEntry(url, db, e)
{
	var format = document.f.format;
	var newFormat = format.options[format.selectedIndex].value;
	window.location = url + "?db=" + db + "&id=" + e + "&format=" + newFormat;
}

function doOp(op)
{
	var w = document.f.add.value;
	
	if (w != "") {
	
		var ix = document.f.fld;

		if (ix.selectedIndex >= 1) {
			w = ix.options[ix.selectedIndex].value + ":" + w;
		}

		var wc = document.f.wildcard;
		if (wc.checked) {
			w = w + "*";
		}

		var q = document.f.query.value;
		if (q != "" || op == "NOT") {
			q = q + " " + op + " " + w;
		}
		else {
			q = w;
		}
		
		document.f.query.value = q;
	}
}

var
	gLastClicked, gClickedWithShift;

function doMouseDownOnCheckbox(event)
{
	gClickedWithShift = event.shiftKey;
	return true;
}

function doClickSelect(id)
{
	var cb = document.getElementById(id);
	var list = document.getElementById('list').getElementsByTagName('input');
	
	if (cb && cb.checked)
	{
		var ix1, ix2;
		
		ix2 = gLastClicked;
		for (i = 0; i < list.length; ++i)
		{
			if (list[i].id == id || list[i].id == gLastClicked)
			{
				ix1 = i;
				break;
			}
		}
		gLastClicked = ix1;
		
		if (ix1 != ix2 && ix2 >= 0 && gClickedWithShift)
		{
			if (ix1 > ix2)
			{
				var tmp = ix1;
				ix1 = ix2;
				ix2 = tmp;
			}
			
			for (i = ix1; i < ix2; ++i)
			{
				list[i].checked = true;
			}
		}
	}
	else
	{
		gLastClicked = -1;
	}

	return true;
}

/*
	for the extended search page
 */

var

	gNextRowNr = 2,

	gSubmitUrl = "";


function msrLoad(url)
{
	gNextRowNr = 2;
	gSubmitUrl = url;
	
	addField();
	addField();
	addField();
}

function addField()
{
	var tbl = document.getElementById('tabel');

	var td = document.getElementById('rij').cloneNode(true);
	
	var input = td.getElementsByTagName('input');
	if (input.length == 1)
	{
		input[0].value = '';
	}
	
	var tr = tbl.insertRow(gNextRowNr++);
	tr.appendChild(td);
}

function changeDb(url)
{
	var db = document.f.db;
	window.location = url + "?db=" + db.options[db.selectedIndex].value + "&extended=1";
}

function doExtSearch()
{
	var input = document.getElementsByTagName('input');
	var indx = document.getElementsByTagName('select');
	var query = '';
	
	var indexNr = 1;
	
	for (i = 0; i < input.length; ++i)
	{
		if (input[i].name == 'fld')
		{
			if (input[i].value.length > 0)
			{
				if (query.length > 0)
					query += ' ';
				
				var ix = indx[indexNr];
				
				if (ix.selectedIndex > 0)
					query += ix.options[ix.selectedIndex].value + ':' + input[i].value;
				else
					query += input[i].value;
			}

			++indexNr;
		}
	}
	
	var db = document.f.db;
	window.location =
		gSubmitUrl + "?db=" + db.options[db.selectedIndex].value +
		'&query=' + escape(query);
}

// [Cookie] Sets value in a cookie
function setCookie(cookieName, cookieValue, cookiePath, cookieExpires)
{
	document.cookie =
		escape(cookieName) + '=' + escape(cookieValue) +
		((cookiePath) ? "; path=" + cookiePath : "") +
		((cookieExpires) ? "; expires=" + cookieExpires : "");
};

function toggleClustalColors() {
	var d = document.getElementById('clustal');
	var l = document.getElementById('clustal_show_hide_button');

	if (d.className == 'clustal') {
		d.className = '';
		l.innerHTML = 'show colors';
	}
	else {
		d.className = 'clustal';
		l.innerHTML = 'hide colors';
	}
}

function toggleClustalWrap() {
	var w = document.getElementById('wrapped');
	var u = document.getElementById('unwrapped');
	var l = document.getElementById('clustal_wrap_button');

	if (w.style.display == 'none') {
		w.style.display = '';
		u.style.display = 'none';
		l.innerHTML = 'show unwrapped';
	}
	else {
		w.style.display = 'none';
		u.style.display = '';
		l.innerHTML = 'show wrapped';
	}
}
