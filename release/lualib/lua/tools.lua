local M = {}
M.version = "1.0"

--通过URL获取baseURL
function M.getBaseURL(url)
  --过滤处理baseURL
  --简单格式化URL
  local s, e=string.find(url, ".-://.*/")
  if(s==nil)
  then
	url=url.."/"
  end
  --拼接过后还是无法匹配的话证明baseURL不是url，退出
  s, e=string.find(url, ".-://.*/")
  if(s==nil)
  then
	return -1
  end
  --处理http://www.pcunions.com/news这种情况
  local str=string.sub(url, e+1, string.len(url))
  if((string.find(str, "?")~=nil or string.find(str, "&")~=nil or string.find(str, "#")~=nil or string.find(str, "%.")~=nil)==false)
  then
	url=url.."/"
  end

  --获取baseURL
  s, e=string.find(url, ".-://.*/")
  if(s==nil)
  then
	return -1
  end
  --组合baseURL字符串
  local baseURL=string.sub(url, s, e)
  return baseURL
end

--通过baseURL获取rootURL
function M.getRootURL(baseURL)
  --获取rootURL
  local s, e=string.find(baseURL, ".-://.-/")
  local rootURL=string.sub(baseURL, s, e)
  return rootURL
end

--获取上一级目录
function M.dirname(path, times)
  local i
  local s, e
  --此时由于baseURL格式化过，所以多一个/，会被识别为多一个time,所以循环为times+1
  for i=1, times+1, 1 do
    path=string.reverse(path)
    s, e=string.find(path, ".-/")
    path=string.sub(path, e+1, string.len(path))
    path=string.reverse(path)
  end
  return path.."/"
end

--格式化附件url
function M.format_attachment_url(url, baseURL)
  --得到rootURL
  local rootURL
  rootURL=M.getRootURL(baseURL)
  if(rootURL==-1)
  then
	return -1
  end

  --判断是url还是路径path
  local s, e=string.find(url, "://")
  if(s~=nil)
  then
	return url
  end

  --判断是开头是否是/
  s, e=string.find(url, "/")
  if(s==1)
  then
	  s, e=string.find(url, "//")
	  if(s~=1)
	  then
		url=rootURL..string.sub(url, 2, string.len(url))
		return url
	  else
		return -1
	  end
  end

  --判断是开头是否是./
  s, e=string.find(url, "./")
  if(s==1)
  then
	  url=baseURL..string.sub(url, 3, string.len(url))
	  return url
  end

  --判断是开头是否是../
  local times=0--times为出现../的次数
  local urlTmp=url--临时url，同时也是附件的文件名
  while(string.find(urlTmp, "../")==1)
  do
    times=times+1
    urlTmp=string.sub(urlTmp, 4, string.len(urlTmp))
  end
  url=M.dirname(baseURL, times)..urlTmp

  return url
end

return M;
