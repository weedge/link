LinkDistributionRetMethod这个项目前期蒋海平根据樊老师的论文《Link Distribution Dependency Model for Document Retrieval》开发的。
1.模型中的统计参数是通过读取lemur中原有倒排索引keyfileIncIndex文件中的信息，进行统计动态生成。
2.模型中的连接信息通过lemur中的原有倒排索引生成，时间效率比较低。
3.参数见link_distribution_param

