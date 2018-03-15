# biliquery_db
Bilibili comment hash to user id rainbow table system


# Generate table
```php -f gen.php 1 300000000 > table```


# Run server
```
pacman -S libev 
make
./biliquery
```
大约只要 <10MB 的内存，在 SSD 上每秒大概可以承受几十万请求，当然也可以改成纯内存的能上千万，不过没啥意义。

# API ( no sla, 300billon )

http://biliquery.typcn.com/api/user/hash/