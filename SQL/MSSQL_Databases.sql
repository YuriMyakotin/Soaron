/****** Object:  Table [dbo].[AreaGroups]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[AreaGroups](
	[GroupNumber] [int] NOT NULL,
	[GroupDescription] [nvarchar](255) NOT NULL,
 CONSTRAINT [aaaaadbo_AreaGroups2_PK] PRIMARY KEY NONCLUSTERED 
(
	[GroupNumber] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[AreaLinks]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[AreaLinks](
	[AreaId] [int] NOT NULL,
	[LinkId] [int] NOT NULL,
	[AllowRead] [bit] NOT NULL DEFAULT ((0)),
	[AllowWrite] [bit] NOT NULL DEFAULT ((0)),
	[Mandatory] [bit] NOT NULL DEFAULT ((0)),
	[Passive] [bit] NOT NULL DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_AreaLinks2_PK] PRIMARY KEY CLUSTERED 
(
	[AreaId] ASC,
	[LinkId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[AutoLinkingAreas]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[AutoLinkingAreas](
	[AreaID] [int] NOT NULL,
	[Mandatory] [bit] NOT NULL,
 CONSTRAINT [PK_AutoLinkingAreas_1] PRIMARY KEY CLUSTERED 
(
	[AreaID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[BigStringTable]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[BigStringTable](
	[StrName] [nvarchar](255) NOT NULL,
	[StrValue] [ntext] NULL,
 CONSTRAINT [aaaaadbo_BigStringTable2_PK] PRIMARY KEY NONCLUSTERED 
(
	[StrName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
/****** Object:  Table [dbo].[DupeBase]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[DupeBase](
	[AreaId] [int] NOT NULL,
	[MsgId] [nvarchar](127) NOT NULL,
	[AddTime] [date] NOT NULL CONSTRAINT [DF_DupeBase_AddTime]  DEFAULT (getdate()),
 CONSTRAINT [PK_DupeBase] PRIMARY KEY CLUSTERED 
(
	[AreaId] ASC,
	[MsgId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[DupesReport]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[DupesReport](
	[DupeID] [int] NOT NULL,
	[AreaId] [int] NOT NULL,
	[FromLinkId] [int] NOT NULL,
	[FromName] [nvarchar](36) NOT NULL,
	[ToName] [nvarchar](36) NOT NULL,
	[CreateTime] [datetime] NOT NULL,
	[ReceiveTime] [datetime] NOT NULL,
	[Subject] [nvarchar](74) NOT NULL,
	[MsgId] [nvarchar](127) NULL,
	[Path] [varbinary](max) NOT NULL,
	[OriginalMessageId] [int] NULL,
 CONSTRAINT [PK_DupesReport] PRIMARY KEY CLUSTERED 
(
	[DupeID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[EchoAreas]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[EchoAreas](
	[AreaId] [int] NOT NULL,
	[AreaName] [nvarchar](80) NOT NULL,
	[AreaDescr] [nvarchar](255) NULL,
	[AreaGroup] [int] NOT NULL,
	[UseAka] [int] NOT NULL,
	[CreatedByLink] [int] NULL,
	[LastMsgDate] [datetime] NULL,
	[IgnoreSeenby] [bit] NOT NULL CONSTRAINT [DF_EchoAreas_IgnoreSeenby]  DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_EchoAreas2_PK] PRIMARY KEY NONCLUSTERED 
(
	[AreaId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING ON

GO
/****** Object:  Index [IX_EchoAreas]    Script Date: 28.08.2014 20:13:26 ******/
CREATE UNIQUE CLUSTERED INDEX [IX_EchoAreas] ON [dbo].[EchoAreas]
(
	[AreaName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, IGNORE_DUP_KEY = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO
/****** Object:  Table [dbo].[EchoMessages]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[EchoMessages](
	[MessageID] [int] NOT NULL,
	[AreaId] [int] NOT NULL,
	[FromLinkId] [int] NULL CONSTRAINT [DF_Test_FromLinkId]  DEFAULT ((0)),
	[FromName] [nvarchar](36) NOT NULL,
	[ToName] [nvarchar](36) NOT NULL,
	[FromZone] [smallint] NOT NULL,
	[FromNet] [smallint] NOT NULL,
	[FromNode] [smallint] NOT NULL,
	[FromPoint] [smallint] NOT NULL,
	[CreateTime] [datetime] NOT NULL,
	[ReceiveTime] [datetime] NOT NULL,
	[Subject] [nvarchar](74) NOT NULL,
	[MsgId] [nvarchar](127) NULL,
	[ReplyTo] [nvarchar](127) NULL,
	[Recv] [bit] NOT NULL CONSTRAINT [DF_Test_Recv]  DEFAULT ((0)),
	[Scanned] [bit] NOT NULL CONSTRAINT [DF_Test_Scanned]  DEFAULT ((0)),
	[OriginalSize] [int] NOT NULL,
	[Path] [varbinary](max) NULL,
	[SeenBy] [varbinary](max) NULL,
	[MsgText] [varbinary](max) NULL,
 CONSTRAINT [PK_Test] PRIMARY KEY CLUSTERED 
(
	[MessageID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[FileOutbound]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[FileOutbound](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[LinkID] [int] NOT NULL,
	[FileName] [nvarchar](260) NOT NULL,
	[KillFileAfterSent] [bit] NOT NULL,
	[Delayed] [bit] NOT NULL,
 CONSTRAINT [PK_FileOutbound] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[GroupPermissions]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[GroupPermissions](
	[LinkId] [int] NOT NULL,
	[AreaGroup] [int] NOT NULL,
	[AllowRead] [bit] NOT NULL DEFAULT ((0)),
	[AllowWrite] [bit] NOT NULL DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_GroupPermissions2_PK] PRIMARY KEY NONCLUSTERED 
(
	[LinkId] ASC,
	[AreaGroup] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[IntTable]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[IntTable](
	[ValueName] [nvarchar](255) NOT NULL,
	[Value] [int] NOT NULL,
 CONSTRAINT [PK_IntTable] PRIMARY KEY CLUSTERED 
(
	[ValueName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[LinkRequest]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[LinkRequest](
	[zone] [smallint] NOT NULL,
	[net] [smallint] NOT NULL,
	[node] [smallint] NOT NULL,
	[Pwd1] [nvarchar](12) NOT NULL,
	[Pwd2] [nvarchar](12) NOT NULL,
	[Ip] [nvarchar](17) NOT NULL,
	[RegDate] [datetime] NULL,
 CONSTRAINT [aaaaadbo_LinkRequest2_PK] PRIMARY KEY NONCLUSTERED 
(
	[zone] ASC,
	[net] ASC,
	[node] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Links]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[Links](
	[LinkID] [int] NOT NULL,
	[Zone] [smallint] NOT NULL,
	[Net] [smallint] NOT NULL,
	[Node] [smallint] NOT NULL,
	[Point] [smallint] NOT NULL,
	[UseAka] [int] NOT NULL,
	[LinkType] [smallint] NOT NULL CONSTRAINT [DF_Links_LinkType]  DEFAULT ((1)),
	[SessionPassword] [nvarchar](64) NULL,
	[PktPassword] [varchar](9) NULL,
	[AreaFixPassword] [nvarchar](64) NULL,
	[Ip] [nvarchar](256) NULL,
	[MaxPktSize] [int] NOT NULL,
	[MaxArchiveSize] [int] NOT NULL,
	[LinkDescription] [nvarchar](256) NULL,
	[DefaultGroup] [int] NOT NULL,
	[DialOut] [bit] NOT NULL CONSTRAINT [DF__dbo_Links__DialO__2D27B809]  DEFAULT ((0)),
	[UsePktPassword] [bit] NOT NULL CONSTRAINT [DF__dbo_Links__UsePk__31EC6D26]  DEFAULT ((0)),
	[AllowAutoCreate] [bit] NOT NULL CONSTRAINT [DF_Links_AllowAutoCreate]  DEFAULT ((1)),
	[PassiveLink] [bit] NOT NULL CONSTRAINT [DF__dbo_Links__Passi__34C8D9D1]  DEFAULT ((0)),
	[LastSessionTime] [datetime] NULL,
	[TotalSessions] [int] NOT NULL CONSTRAINT [DF_Links_TotalSessions]  DEFAULT ((0)),
	[ThisDaySessions] [int] NOT NULL,
	[LastSessionType] [int] NULL,
	[NetmailDirect] [bit] NOT NULL CONSTRAINT [DF__dbo_Links__Netma__35BCFE0A]  DEFAULT ((0)),
	[NextArchiveExt] [int] NOT NULL CONSTRAINT [DF__dbo_Links__IsBon__36B12243]  DEFAULT ((0)),
	[IpPort] [int] NOT NULL CONSTRAINT [DF_Links_IpPort]  DEFAULT ((0)),
	[LastCallStatus] [tinyint] NOT NULL CONSTRAINT [DF_Links_LastSessionStatus]  DEFAULT ((0)),
	[isBusy] [bit] NOT NULL CONSTRAINT [DF_Links_isBusy]  DEFAULT ((0)),
	[isCalling] [bit] NOT NULL CONSTRAINT [DF_Links_isCalling]  DEFAULT ((0)),
	[FailedCallsCount] [smallint] NOT NULL CONSTRAINT [DF_Links_FailedCalls]  DEFAULT ((0)),
	[HoldUntil] [datetime] NULL CONSTRAINT [DF_Links_HoldUntil]  DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_Links2_PK] PRIMARY KEY CLUSTERED 
(
	[LinkID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[LinksAKAs]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[LinksAKAs](
	[LinkID] [int] NOT NULL,
	[Zone] [smallint] NOT NULL,
	[Net] [smallint] NOT NULL,
	[Node] [smallint] NOT NULL,
	[Point] [smallint] NOT NULL CONSTRAINT [DF_LinksAKAs_Point]  DEFAULT ((0))
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[ListenPorts]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
CREATE TABLE [dbo].[ListenPorts](
	[IpAddr] [varchar](50) NOT NULL,
	[isIPV6] [bit] NOT NULL,
	[Comments] [nvarchar](255) NULL
) ON [PRIMARY]

GO
SET ANSI_PADDING OFF
GO
/****** Object:  Table [dbo].[Logs]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Logs](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[ThreadID] [int] NOT NULL,
	[LogTime] [datetime] NOT NULL,
	[LogText] [nvarchar](255) NOT NULL,
 CONSTRAINT [PK_Logs] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Modules]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Modules](
	[ModuleName] [nvarchar](255) NOT NULL,
	[ModuleFileName] [nvarchar](255) NOT NULL,
	[EventName] [nvarchar](255) NOT NULL,
 CONSTRAINT [aaaaadbo_Modules2_PK] PRIMARY KEY NONCLUSTERED 
(
	[ModuleName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[MyAka]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[MyAka](
	[MyAkaId] [int] NOT NULL,
	[Zone] [smallint] NOT NULL,
	[Net] [smallint] NOT NULL,
	[Node] [smallint] NOT NULL,
	[Point] [smallint] NOT NULL,
	[AkaDescription] [nvarchar](255) NOT NULL,
 CONSTRAINT [aaaaadbo_MyAka2_PK] PRIMARY KEY NONCLUSTERED 
(
	[MyAkaId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Netmail]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Netmail](
	[MessageID] [int] NOT NULL,
	[FromZone] [smallint] NOT NULL,
	[FromNet] [smallint] NOT NULL,
	[FromNode] [smallint] NOT NULL,
	[FromPoint] [smallint] NOT NULL,
	[ToZone] [smallint] NOT NULL,
	[ToNet] [smallint] NOT NULL,
	[ToNode] [smallint] NOT NULL,
	[ToPoint] [smallint] NOT NULL,
	[CreateTime] [datetime] NOT NULL,
	[ReceiveTime] [datetime] NOT NULL,
	[FromName] [nvarchar](36) NOT NULL,
	[ToName] [nvarchar](36) NOT NULL,
	[Subject] [nvarchar](74) NOT NULL,
	[MsgId] [nvarchar](127) NULL,
	[ReplyTo] [nvarchar](127) NULL,
	[MsgText] [nvarchar](max) NULL,
	[Sent] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmai__Sent__5FB337D6]  DEFAULT ((0)),
	[Recv] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmai__Recv__60A75C0F]  DEFAULT ((0)),
	[Visible] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__Visib__619B8048]  DEFAULT ((0)),
	[Readed] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__Reade__628FA481]  DEFAULT ((0)),
	[KillSent] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__KillS__6383C8BA]  DEFAULT ((0)),
	[Pvt] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmail__Pvt__6477ECF3]  DEFAULT ((0)),
	[FileAttach] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__FileA__656C112C]  DEFAULT ((0)),
	[Arq] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmail__Arq__6754599E]  DEFAULT ((0)),
	[Rrq] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmail__Rrq__68487DD7]  DEFAULT ((0)),
	[ReturnReq] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__Retur__693CA210]  DEFAULT ((0)),
	[Direct] [bit] NOT NULL CONSTRAINT [DF__dbo_Netma__Direc__6A30C649]  DEFAULT ((0)),
	[Cfm] [bit] NOT NULL CONSTRAINT [DF__dbo_Netmail__Cfm__6D0D32F4]  DEFAULT ((0)),
	[Locked] [int] NOT NULL CONSTRAINT [DF__dbo_Netma__Locke__6E01572D]  DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_Netmail2_PK] PRIMARY KEY CLUSTERED 
(
	[MessageID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
/****** Object:  Table [dbo].[NetmailOutbound]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NetmailOutbound](
	[ToLinkID] [int] NOT NULL,
	[MessageID] [int] NOT NULL,
 CONSTRAINT [PK_NetmailOutbound] PRIMARY KEY CLUSTERED 
(
	[ToLinkID] ASC,
	[MessageID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[NetmailRobots]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NetmailRobots](
	[RobotsCode] [smallint] NOT NULL,
	[RobotsName] [nvarchar](255) NOT NULL,
	[RobotsFileName] [nvarchar](255) NOT NULL,
 CONSTRAINT [aaaaadbo_NetmailRobots2_PK] PRIMARY KEY CLUSTERED 
(
	[RobotsCode] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[NetmailRouTable]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NetmailRouTable](
	[RouteThruZone] [smallint] NOT NULL,
	[RouteThruNet] [smallint] NOT NULL,
	[RouteThruNode] [smallint] NOT NULL,
	[RouteDestZone] [smallint] NOT NULL,
	[RouteDestNet] [smallint] NOT NULL,
	[RouteDestNode] [smallint] NOT NULL,
	[processed] [bit] NOT NULL CONSTRAINT [DF_NetmailRouTable_processed]  DEFAULT ((0)),
 CONSTRAINT [PK_NetmailRouTable] PRIMARY KEY CLUSTERED 
(
	[RouteThruZone] ASC,
	[RouteThruNet] ASC,
	[RouteThruNode] ASC,
	[RouteDestZone] ASC,
	[RouteDestNet] ASC,
	[RouteDestNode] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[NetmailTruTable]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NetmailTruTable](
	[ToZone] [smallint] NOT NULL,
	[ToNet] [smallint] NOT NULL,
	[ToNode] [smallint] NOT NULL,
	[TrustedAddrZone] [smallint] NOT NULL,
	[TrustedAddrNet] [smallint] NOT NULL,
	[TrustedAddrNode] [smallint] NOT NULL,
	[processed] [bit] NOT NULL CONSTRAINT [DF_TruFiles_processed]  DEFAULT ((0)),
 CONSTRAINT [PK_NetmailTruTable] PRIMARY KEY CLUSTERED 
(
	[ToZone] ASC,
	[ToNet] ASC,
	[ToNode] ASC,
	[TrustedAddrZone] ASC,
	[TrustedAddrNet] ASC,
	[TrustedAddrNode] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[NewAreaLinks]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[NewAreaLinks](
	[AreaGroup] [int] NOT NULL,
	[LinkId] [int] NOT NULL,
	[AllowRead] [bit] NOT NULL DEFAULT ((0)),
	[AllowWrite] [bit] NOT NULL DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_NewAreaLinks2_PK] PRIMARY KEY CLUSTERED 
(
	[AreaGroup] ASC,
	[LinkId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Nodelist]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Nodelist](
	[Zone] [smallint] NOT NULL,
	[Net] [smallint] NOT NULL,
	[Node] [smallint] NOT NULL,
	[SysopName] [nvarchar](255) NOT NULL,
	[Hub] [bit] NOT NULL DEFAULT ((0)),
	[Hold] [bit] NOT NULL DEFAULT ((0)),
	[Down] [bit] NOT NULL DEFAULT ((0)),
	[Pvt] [bit] NOT NULL DEFAULT ((0)),
	[Host] [bit] NOT NULL DEFAULT ((0)),
	[Region] [bit] NOT NULL DEFAULT ((0)),
	[ZC] [bit] NOT NULL DEFAULT ((0)),
	[HubNode] [smallint] NOT NULL,
	[processed] [bit] NOT NULL CONSTRAINT [DF_Nodelist_processed]  DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_Nodelist2_PK] PRIMARY KEY NONCLUSTERED 
(
	[Zone] ASC,
	[Net] ASC,
	[Node] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[OutBound]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[OutBound](
	[ToLink] [int] NOT NULL,
	[MessageId] [int] NOT NULL,
	[Status] [tinyint] NOT NULL,
 CONSTRAINT [aaaaadbo_OutBound2_PK] PRIMARY KEY NONCLUSTERED 
(
	[ToLink] ASC,
	[MessageId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[PartiallyReceivedFiles]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[PartiallyReceivedFiles](
	[LinkID] [int] NOT NULL,
	[FileName] [nvarchar](260) NOT NULL,
	[FileSize] [bigint] NOT NULL,
	[FileLastChangedTime] [bigint] NOT NULL,
 CONSTRAINT [PK_PartiallyReceivedFiles] PRIMARY KEY CLUSTERED 
(
	[LinkID] ASC,
	[FileName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[RobotsNames]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[RobotsNames](
	[RobotsName] [nvarchar](36) NOT NULL,
	[ActionCode] [smallint] NOT NULL,
 CONSTRAINT [aaaaadbo_RobotsNames2_PK] PRIMARY KEY CLUSTERED 
(
	[RobotsName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Routing]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Routing](
	[LinkId] [int] NOT NULL,
	[Zone] [smallint] NOT NULL,
	[Net] [smallint] NOT NULL,
	[Node] [smallint] NOT NULL,
	[Static] [bit] NOT NULL DEFAULT ((0)),
 CONSTRAINT [aaaaadbo_Routing2_PK] PRIMARY KEY NONCLUSTERED 
(
	[LinkId] ASC,
	[Zone] ASC,
	[Net] ASC,
	[Node] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[RulesBase]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[RulesBase](
	[RulesId] [int] NOT NULL,
	[Areaid] [int] NOT NULL,
	[FromName] [nvarchar](36) NOT NULL,
	[ToName] [nvarchar](36) NOT NULL,
	[Subject] [nvarchar](74) NOT NULL,
	[RulesText] [nvarchar](max) NULL,
 CONSTRAINT [aaaaadbo_RulesBase2_PK] PRIMARY KEY NONCLUSTERED 
(
	[RulesId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]

GO
/****** Object:  Table [dbo].[Schedule]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[Schedule](
	[TimerID] [int] IDENTITY(1,1) NOT NULL,
	[ScheduleType] [tinyint] NOT NULL,
	[Enabled] [bit] NOT NULL CONSTRAINT [DF_Schedule_Enabled]  DEFAULT ((1)),
	[Day] [tinyint] NOT NULL,
	[Starttime] [time](7) NOT NULL,
	[Period] [int] NOT NULL,
	[EventObjectName] [nvarchar](255) NOT NULL,
	[Comments] [nvarchar](255) NULL,
 CONSTRAINT [PK_Schedule] PRIMARY KEY CLUSTERED 
(
	[TimerID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[StringTable]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[StringTable](
	[StrName] [nvarchar](255) NOT NULL,
	[StrValue] [nvarchar](255) NOT NULL,
 CONSTRAINT [aaaaadbo_StringTable2_PK] PRIMARY KEY NONCLUSTERED 
(
	[StrName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Table [dbo].[WeeklyEchoStat]    Script Date: 28.08.2014 20:13:26 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[WeeklyEchoStat](
	[AreaId] [int] NOT NULL,
	[Messages] [int] NOT NULL,
 CONSTRAINT [aaaaadbo_WeeklyEchoStat2_PK] PRIMARY KEY NONCLUSTERED 
(
	[AreaId] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
/****** Object:  Index [IX_Links]    Script Date: 28.08.2014 20:13:26 ******/
CREATE UNIQUE NONCLUSTERED INDEX [IX_Links] ON [dbo].[Links]
(
	[Zone] ASC,
	[Net] ASC,
	[Node] ASC,
	[Point] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, IGNORE_DUP_KEY = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO
ALTER TABLE [dbo].[FileOutbound] ADD  CONSTRAINT [DF_FileOutbound_KillFileAfterSent]  DEFAULT ((0)) FOR [KillFileAfterSent]
GO
ALTER TABLE [dbo].[FileOutbound] ADD  CONSTRAINT [DF_FileOutbound_Delayed]  DEFAULT ((0)) FOR [Delayed]
GO
ALTER TABLE [dbo].[OutBound] ADD  CONSTRAINT [DF_OutBound_Status]  DEFAULT ((0)) FOR [Status]
GO
ALTER TABLE [dbo].[AreaLinks]  WITH CHECK ADD  CONSTRAINT [FK_AreaLinks_EchoAreas] FOREIGN KEY([AreaId])
REFERENCES [dbo].[EchoAreas] ([AreaId])
GO
ALTER TABLE [dbo].[AreaLinks] CHECK CONSTRAINT [FK_AreaLinks_EchoAreas]
GO
ALTER TABLE [dbo].[AreaLinks]  WITH CHECK ADD  CONSTRAINT [FK_AreaLinks_Links] FOREIGN KEY([LinkId])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[AreaLinks] CHECK CONSTRAINT [FK_AreaLinks_Links]
GO
ALTER TABLE [dbo].[AutoLinkingAreas]  WITH CHECK ADD  CONSTRAINT [FK_AutoLinkingAreas_EchoAreas] FOREIGN KEY([AreaID])
REFERENCES [dbo].[EchoAreas] ([AreaId])
GO
ALTER TABLE [dbo].[AutoLinkingAreas] CHECK CONSTRAINT [FK_AutoLinkingAreas_EchoAreas]
GO
ALTER TABLE [dbo].[DupeBase]  WITH CHECK ADD  CONSTRAINT [FK_DupeBase_EchoAreas] FOREIGN KEY([AreaId])
REFERENCES [dbo].[EchoAreas] ([AreaId])
GO
ALTER TABLE [dbo].[DupeBase] CHECK CONSTRAINT [FK_DupeBase_EchoAreas]
GO
ALTER TABLE [dbo].[EchoAreas]  WITH CHECK ADD  CONSTRAINT [FK_EchoAreas_AreaGroups] FOREIGN KEY([AreaGroup])
REFERENCES [dbo].[AreaGroups] ([GroupNumber])
GO
ALTER TABLE [dbo].[EchoAreas] CHECK CONSTRAINT [FK_EchoAreas_AreaGroups]
GO
ALTER TABLE [dbo].[EchoAreas]  WITH CHECK ADD  CONSTRAINT [FK_EchoAreas_MyAka] FOREIGN KEY([UseAka])
REFERENCES [dbo].[MyAka] ([MyAkaId])
GO
ALTER TABLE [dbo].[EchoAreas] CHECK CONSTRAINT [FK_EchoAreas_MyAka]
GO
ALTER TABLE [dbo].[EchoMessages]  WITH CHECK ADD  CONSTRAINT [FK_Test_EchoAreas] FOREIGN KEY([AreaId])
REFERENCES [dbo].[EchoAreas] ([AreaId])
GO
ALTER TABLE [dbo].[EchoMessages] CHECK CONSTRAINT [FK_Test_EchoAreas]
GO
ALTER TABLE [dbo].[FileOutbound]  WITH CHECK ADD  CONSTRAINT [FK_FileOutbound_Links] FOREIGN KEY([LinkID])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[FileOutbound] CHECK CONSTRAINT [FK_FileOutbound_Links]
GO
ALTER TABLE [dbo].[GroupPermissions]  WITH CHECK ADD  CONSTRAINT [FK_GroupPermissions_AreaGroups] FOREIGN KEY([AreaGroup])
REFERENCES [dbo].[AreaGroups] ([GroupNumber])
GO
ALTER TABLE [dbo].[GroupPermissions] CHECK CONSTRAINT [FK_GroupPermissions_AreaGroups]
GO
ALTER TABLE [dbo].[GroupPermissions]  WITH CHECK ADD  CONSTRAINT [FK_GroupPermissions_Links] FOREIGN KEY([LinkId])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[GroupPermissions] CHECK CONSTRAINT [FK_GroupPermissions_Links]
GO
ALTER TABLE [dbo].[Links]  WITH CHECK ADD  CONSTRAINT [FK_Links_MyAka] FOREIGN KEY([UseAka])
REFERENCES [dbo].[MyAka] ([MyAkaId])
GO
ALTER TABLE [dbo].[Links] CHECK CONSTRAINT [FK_Links_MyAka]
GO
ALTER TABLE [dbo].[LinksAKAs]  WITH CHECK ADD  CONSTRAINT [FK_LinksAKAs_Links] FOREIGN KEY([LinkID])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[LinksAKAs] CHECK CONSTRAINT [FK_LinksAKAs_Links]
GO
ALTER TABLE [dbo].[NetmailOutbound]  WITH CHECK ADD  CONSTRAINT [FK_NetmailOutbound_Links] FOREIGN KEY([ToLinkID])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[NetmailOutbound] CHECK CONSTRAINT [FK_NetmailOutbound_Links]
GO
ALTER TABLE [dbo].[NetmailOutbound]  WITH CHECK ADD  CONSTRAINT [FK_NetmailOutbound_Netmail] FOREIGN KEY([MessageID])
REFERENCES [dbo].[Netmail] ([MessageID])
GO
ALTER TABLE [dbo].[NetmailOutbound] CHECK CONSTRAINT [FK_NetmailOutbound_Netmail]
GO
ALTER TABLE [dbo].[NewAreaLinks]  WITH CHECK ADD  CONSTRAINT [FK_NewAreaLinks_AreaGroups] FOREIGN KEY([AreaGroup])
REFERENCES [dbo].[AreaGroups] ([GroupNumber])
GO
ALTER TABLE [dbo].[NewAreaLinks] CHECK CONSTRAINT [FK_NewAreaLinks_AreaGroups]
GO
ALTER TABLE [dbo].[NewAreaLinks]  WITH CHECK ADD  CONSTRAINT [FK_NewAreaLinks_Links] FOREIGN KEY([LinkId])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[NewAreaLinks] CHECK CONSTRAINT [FK_NewAreaLinks_Links]
GO
ALTER TABLE [dbo].[OutBound]  WITH CHECK ADD  CONSTRAINT [FK_OutBound_EchoMessages] FOREIGN KEY([MessageId])
REFERENCES [dbo].[EchoMessages] ([MessageID])
GO
ALTER TABLE [dbo].[OutBound] CHECK CONSTRAINT [FK_OutBound_EchoMessages]
GO
ALTER TABLE [dbo].[OutBound]  WITH CHECK ADD  CONSTRAINT [FK_OutBound_Links] FOREIGN KEY([ToLink])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[OutBound] CHECK CONSTRAINT [FK_OutBound_Links]
GO
ALTER TABLE [dbo].[PartiallyReceivedFiles]  WITH CHECK ADD  CONSTRAINT [FK_PartiallyReceivedFiles_Links] FOREIGN KEY([LinkID])
REFERENCES [dbo].[Links] ([LinkID])
GO
ALTER TABLE [dbo].[PartiallyReceivedFiles] CHECK CONSTRAINT [FK_PartiallyReceivedFiles_Links]
GO
ALTER TABLE [dbo].[WeeklyEchoStat]  WITH CHECK ADD  CONSTRAINT [FK_WeeklyEchoStat_EchoAreas] FOREIGN KEY([AreaId])
REFERENCES [dbo].[EchoAreas] ([AreaId])
GO
ALTER TABLE [dbo].[WeeklyEchoStat] CHECK CONSTRAINT [FK_WeeklyEchoStat_EchoAreas]
GO
