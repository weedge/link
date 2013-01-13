1.开发环境vs2008+windows xp   
2.工程：
1) ParseToLink  查询连接生成
2) BuildLinkInfo  连接信息的生成，包括连接CDF和词对MI的生成、候选查询连接的生成、连接索引的生成(存在bug)、连接模型(静态读取连接索引文件，没有整合在一个程序中，单独出来称为LinkDistributionRetrieval)。
3) LinkDistributionRetMethod 模型实现。通过lemur中原始倒排索引来统计获取连接信