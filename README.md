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
About 15GB RAM needed

其实我好像有点傻逼了，数据量这么大直接连续内存 int32 大概就可以做到只要 5G 内存左右。。。速度还更快。。。不过懒得改了。。。

# API ( no sla, 300billon )

http://biliquery.typcn.com/api/user/hash/