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
程序本体大约只要 1MB 的内存，在 SSD 上每秒大概可以承受几十万请求，当然也可以改成纯内存的能上千万，不过没啥意义。

冲突表在内存里，但是由于 HASH 冲突非常多，冲突表甚至占了整整 4G 内存，待优化。

# API ( no sla, 300billon )

http://biliquery.typcn.com/api/user/hash/