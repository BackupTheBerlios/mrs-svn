function showBox(id, text)
{
	var p = document.getElementById(id);
	var t = document.getElementById(id + '_text');
	
	if (t.style.display == 'none') {
		t.style.display = '';
		p.innerHTML = 'Hide ' + text;
	}
	else {
		t.style.display = 'none';
		p.innerHTML = 'Show ' + text;
	}
}

function show(id)
{
	var hits = document.getElementById('Hits').getElementsByTagName('tr');
	
	for (i = 2; i < hits.length; ++i) {
		if (hits[i].id == id) {

			if (hits[i].style.display == 'none') {
				hits[i].style.display = '';
			}
			else {
				hits[i].style.display = 'none';
				
				var l = id.length;
				
				while (i < hits.length)
				{
					if (hits[i].id.substr(l) == id && hits[i].id[l] == ':')
						hits[i].style.display = 'none';
					
					++i;
				}
			}
		}
	}
}

function enterItem(id, url)
{
	var item = document.getElementById(id);
	
	if (item)
	{
		item.style.backgroundColor = '#ddf';
		item.style.cursor = 'pointer';
	}
	
	if (url)
		window.status = url;
	
	return true;
}

function leaveItem(id)
{
	var item = document.getElementById(id);
	
	if (item)
		item.style.backgroundColor = 'white';

	return true;
}

function showQuery(url)
{
	window.location = url;
}