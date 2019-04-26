user=cyb70289
pass=xxxxxxxx

while read line; do
  echo $line
  sz=`curl --user "${user}:${pass}" https://api.github.com/repos/$line 2>/dev/null | grep size | tr -dc '[:digit:]'`
  echo ${sz}
done < projects
