/****** Object:  StoredProcedure [dbo].[AddNewLinkRequest]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- 0 все ок
-- 1 узла нет в нодлисте
-- 2 линк уже есть
-- 3 Запрос на линк уже есть
-- 4 hold/down
CREATE PROCEDURE [dbo].[AddNewLinkRequest] @Zone smallint, @Net smallint, @Node smallint, @pwd1 nvarchar(12), @pwd2 nvarchar(12),@Ip nvarchar(17), @result int output,@Sysopname nvarchar(255) output
AS
declare @cnt int,@hold bit,@down bit
BEGIN

	SET NOCOUNT ON;
	select @cnt=count(Zone) from Nodelist where Zone=@Zone and Net=@Net and Node=@Node
	if @cnt=0 
		begin
			select @result=1
			return
		end

	select @Sysopname=SysopName,@hold=Hold,@down=@Down from Nodelist where Zone=@Zone and Net=@Net and Node=@Node
	if (@hold<>0) or (@down<>0)
		begin
			select @result=4
			return
		end

	select @cnt=count(LinkId) from Links where Zone=@Zone and Net=@Net and Node=@Node and Point=0
		if @cnt<>0 
		begin
			select @result=2
			return
		end
			select @cnt=count(Zone) from LinkRequest where Zone=@Zone and Net=@Net and Node=@Node
		if @cnt<>0 
		begin
			select @result=3
			return
		end
		insert into LinkRequest values(@Zone,@Net,@Node,@pwd1,@pwd2,@Ip,getdate())
		select @result=0
		return
	
END

GO
/****** Object:  StoredProcedure [dbo].[GetMsgIdTime]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[GetMsgIdTime] @retval int output
AS
SET NOCOUNT ON;
declare @retval1 int

begin tran
select @retval=datediff(s,'1.1.1970',getdate())
select @retval1=Value from IntTable where ValueName='MsgIdLastTime'
if (@retval>=@retval1)
	select @retval=@retval+1
else
	select @retval=@retval1+1

update IntTable set Value=@retval where ValueName='MsgIdLastTime'
commit tran

return 

GO
/****** Object:  StoredProcedure [dbo].[LinksCount]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE PROCEDURE [dbo].[LinksCount] @LC int output
AS
BEGIN
	 
	SET NOCOUNT ON;

    -- Insert statements for procedure here
	SELECT @LC=count(LinkID) from Links where Point=0 and PassiveLink=0 and LinkType<3
	
END

GO
/****** Object:  StoredProcedure [dbo].[MessagesCount]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- ================================================
create PROCEDURE [dbo].[MessagesCount] @cnt int output
AS
BEGIN
	 
	SET NOCOUNT ON;

    -- Insert statements for procedure here
	SELECT @cnt=count(MessageID) from EchoMessages 
	
END

GO
/****** Object:  StoredProcedure [dbo].[sp_Add_Echomail_Message]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_Add_Echomail_Message](@AreaId int, @FromLinkId int,
 @FromName nvarchar(36), @ToName nvarchar(36), @FromZone smallint, @FromNet smallint,@FromNode smallint,@FromPoint smallint,
 @CreateTime datetime, @Subject nvarchar(74), @MsgId nvarchar(127), @ReplyTo nvarchar(127), @OriginalSize int,@MsgText varbinary(max), @Path varbinary(max), @SeenBy varbinary(max))
AS
declare @MessageId int

BEGIN
	SET NOCOUNT ON;
	update EchoAreas set LastMsgDate=getdate() where AreaId=@AreaId
	begin tran
	select @MessageId=isnull(max(MessageID),0)+1 from EchoMessages
	insert into EchoMessages values(@MessageId, @AreaId, @FromLinkId, @FromName, @ToName,@FromZone, @FromNet, @FromNode, @FromPoint, @CreateTime, getdate(),@Subject, @MsgId,@ReplyTo,1,0,@OriginalSize,@Path, @Seenby, @MsgText)
	if isnull(@MsgId,'')<>'' insert into DupeBase values(@AreaId,@MsgId,GetDate())
	commit tran
	exec sp_EchoStat @AreaId
END

GO
/****** Object:  StoredProcedure [dbo].[sp_Add_Netmail_Message]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_Add_Netmail_Message]
@FromZone smallint, @FromNet smallint, @FromNode smallint, @FromPoint smallint,
@ToZone smallint, @ToNet smallint, @ToNode smallint, @ToPoint smallint,
@CreateTime datetime,
@FromName nvarchar(36),@ToName nvarchar(36),
@Subject nvarchar(74),
@MsgId nvarchar(127), @ReplyTo nvarchar(127),
@MsgText nvarchar(MAX), 
@killsent bit, @pvt bit, @fileattach bit, @arq bit, @rrq bit, @returnreq bit, @direct bit,
@cfm bit, @visible bit, @recv bit
AS
declare @MessageId int, @RedirectToLinkID int
BEGIN
	SET NOCOUNT ON;
if @visible<>0 and @recv<>0
BEGIN
	Select @RedirectToLinkID=LinkID from Links,IntTable where Links.LinkID=IntTable.Value and IntTable.ValueName='RedirectMyNetmailToLinkID'
	Select @RedirectToLinkID=ISNULL(@RedirectToLinkID,0)
	if @RedirectToLinkID<>0
		select @ToZone=Zone,@ToNet=Net,@ToNode=Node,@ToPoint=Point,@visible=0 from Links where LinkID=@RedirectToLinkID

END
begin tran
select @MessageId=isnull(max(messageid),0)+1 from Netmail
insert into Netmail values(@MessageId, @FromZone, @FromNet, @FromNode, @FromPoint,
@ToZone, @ToNet, @ToNode, @ToPoint, @CreateTime, getdate(),@FromName, @ToName,
@Subject, @MsgId, @ReplyTo, @MsgText, 0, @recv, @visible, 0, @killsent, @pvt, @fileattach, @arq, @rrq,
@returnreq, @direct, @cfm,0)


commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_AddDupeReport]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_AddDupeReport] @AreaId int, @FromLinkId int,@FromName nvarchar(36),@ToName nvarchar(36),@CreateTime datetime,@Subject nvarchar(74),@MsgId nvarchar(127),@Path varbinary(max)
AS
BEGIN
	SET NOCOUNT ON;
declare @DupeId int,@OrigMessageId int
declare @IgnoreSeenby bit
select @IgnoreSeenby=IgnoreSeenby from EchoAreas where Areaid=@AreaId
if @IgnoreSeenby<>0 return

begin tran
select @DupeId=isnull(max(DupeId),0)+1 from DupesReport
if isnull(@MsgId,'')='' select @OrigMessageId=0
else select @OrigMessageId=MessageId from EchoMessages where AreaId=@AreaId and MsgId=@MsgId
insert into DupesReport values(@DupeId,@AreaId,@FromLinkId,@FromName,@ToName,@CreateTime,GetDate(),@Subject,@MsgId,@Path,@OrigMessageId)
commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_AddLink]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_AddLink] @zone int, @net int, @node int, @ip nvarchar(256), @LinkId int out
AS
declare @DialOut bit,@len int
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	SET NOCOUNT ON;
	select @len=LEN(isnull(@ip,''))
	if @len=0 select @DialOut=0
	else select @DialOut=1

	begin tran
	Select @LinkId=(ISNULL(Max(LinkId),1))+1 from Links
	insert into Links values (@LinkId,@zone,@net,@node,0,1,1,'default',NULL,'default',@ip,640000,500000,NULL,0,@Dialout,0,1,0,GETDATE(),0,0,0,1,0,0,0,0,0,0,0)
	insert into GroupPermissions values(@LinkId,0,1,1)
	insert into GroupPermissions values(@LinkId,2,1,1)
	insert into AreaLinks select AreaId,@LinkId,1,1,Mandatory,0 from AutoLinkingAreas
	commit tran
	delete from LinkRequest where zone=@zone and net=@net and node=@node

END

GO
/****** Object:  StoredProcedure [dbo].[sp_CheckEchomailMessage]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_CheckEchomailMessage] 
@LinkId int, @AreaName nvarchar(255),@AreaId int output
/*
0 - все ок
1 - создана новая эха
2 - пользователь не имеет права создавать эху/писать в нее.
3 - подписан на уже существующую эху

*/
AS
declare @allowread bit,@allowwrite bit, @mandatory bit,@passive bit, @allowautocreate bit
declare @AreaGroup int, @cnt int
BEGIN
SET NOCOUNT ON;
select @AreaId=0
select @AreaName=upper(ltrim(rtrim(@AreaName)))

select @AreaId=AreaId from EchoAreas where AreaName=@AreaName
select @AreaId=isnull(@AreaId,0)
if @AreaId=0 /*эхи нет*/
	begin
		select @allowautocreate=allowautocreate from Links where LinkId=@LinkId
		if @allowautocreate<>0
			begin
			exec @AreaId=sp_CreateEchoArea @AreaName,@LinkId
			return 1
			end
		else
		begin
			return 2
		end

	end
else
	begin


	select @cnt=count(AreaId) from AreaLinks where AreaId=@AreaId and LinkId=@LinkId and AllowWrite<>0

	if @cnt=0
		begin
		/*нет активной подписки, разрешающей запись*/
		select @allowautocreate=allowautocreate from Links where LinkId=@LinkId
		if @allowautocreate=0 return 2
		else
			begin
			select @cnt=count(AreaId) from AreaLinks where AreaId=@AreaId and LinkId=@LinkId
			if @cnt<>0 return 2
			/**/
			select @AreaGroup=AreaGroup from Echoareas where AreaId=@AreaId
			select @allowread=allowread,@allowwrite=allowwrite from GroupPermissions where LinkId=@LinkId and AreaGroup=@AreaGroup
			select @allowread=isnull(@allowread,0)
			select @allowwrite=isnull(@allowwrite,0)
			if ((@allowwrite=0) and (@allowread=0)) return 2
			/* добавляем подписку*/
			insert into AreaLinks values(@AreaId,@LinkId,@allowread,@allowwrite,0,0)
			return 3
			end
		end
	else
		begin
			update AreaLinks set Passive=0 where AreaId=@AreaId and LinkId=@LinkId
			return 0
		end
	end	
end	








GO
/****** Object:  StoredProcedure [dbo].[sp_CheckInNodelist]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_CheckInNodelist]
	-- Add the parameters for the stored procedure here
	
AS
BEGIN


declare @linkid int, @zone smallint, @net smallint, @node smallint,@z1 smallint
delete from LinksNotInNodelist
declare chk cursor local read_only for select linkid, zone, net, node from Links where point=0 and PassiveLink=0
open chk
fetch next from chk into @linkid, @zone, @net, @node
WHILE @@FETCH_STATUS = 0
BEGIN
select @z1=zone from Nodelist where Zone=@zone and Net=@net and Node=@node
select @z1=isnull(@z1, 0)
if @z1=0
	begin
	insert into LinksNotInNodelist values(@linkid)
	end

fetch next from chk into @linkid, @zone, @net, @node
end
close chk
deallocate chk

END

GO
/****** Object:  StoredProcedure [dbo].[sp_CheckNetmailMsg]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_CheckNetmailMsg]
@FromZone smallint, @FromNet smallint, @FromNode smallint, @FromPoint smallint,
@ToZone smallint, @ToNet smallint, @ToNode smallint, @ToPoint smallint,
@ToName nvarchar(36), @Subject nvarchar(74),@FromLinkID int output, @ActionCode int output
AS
declare @retval int, @MyAkaId int
/*
0 - не ко мнe
1 - ко мне
2 - к моим пойнтам
3 - к роботу, не от линка или неправильный пароль
4 - к роботу, от линка с правильным паролем
*/
BEGIN
	SET NOCOUNT ON;

select @MyAkaId=MyAkaId from MyAka where Zone=@ToZone and Net=@ToNet and Node=@ToNode and Point=0 and @ToPoint<>0
select @MyAkaId=IsNull(@MyAkaId,0)
if @MyAkaId<>0 return 2

select @MyAkaId=MyAkaId from MyAka where Zone=@ToZone and Net=@ToNet and Node=@ToNode and Point=@ToPoint
select @MyAkaId=IsNull(@MyAkaId,0)
if @MyAkaId=0 return 0

select @ActionCode=ActionCode from RobotsNames where Robotsname=@Toname
select @ActionCode=isnull(@ActionCode, 0)
if @ActionCode=0 return 1

Select @FromLinkID=LinkId from Links where Zone=@FromZone and Net=@FromNet and Node=@FromNode and Point=@FromPoint and AreaFixPassword=@Subject
select @FromLinkID=isnull(@FromLinkID,0)
if @FromLinkId=0 return 3
else return 4
  
END


GO
/****** Object:  StoredProcedure [dbo].[sp_CheckPktLink]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_CheckPktLink] 
@FromZone smallint, @FromNet smallint, @FromNode smallint, @FromPoint smallint,
@ToZone smallint, @ToNet smallint, @ToNode smallint, @ToPoint smallint,
@PktPwd varchar(9),@PktFromLinkId int output,  @PktToMyAkaId int output
AS
declare @UsePktPassword bit,@tmp int;
BEGIN
	SET NOCOUNT ON;
select @PktToMyAkaId=MyAkaId from MyAka where Zone=@ToZone and Net=@ToNet and Node=@ToNode and Point=@ToPoint
if isnull(@PktToMyAkaId,0)=0 return 3

Select @PktFromLinkID=LinkId from Links where Zone=@FromZone and Net=@FromNet and Node=@FromNode and Point=@FromPoint
if isnull(@PktFromLinkId,0)=0
begin
	Select @PktFromLinkID=LinkId from LinksAKAs where Zone=@FromZone and Net=@FromNet and Node=@FromNode and Point=@FromPoint
	if isnull(@PktFromLinkId,0)=0
	return 4
end
select @UsePktPassword=UsePktPassword from Links where LinkID=@PktFromLinkId
if @UsePktPassword<>0
begin
select @tmp=count(*) from Links where LinkID=@PktFromLinkId and PktPassword=@PktPwd
if @tmp=0 return 6
end



return 1

END

GO
/****** Object:  StoredProcedure [dbo].[sp_CreateEchoArea]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE  [dbo].[sp_CreateEchoArea] @AreaName nvarchar(80),@LinkId int
AS
declare @AreaId int,@AreaGroup int,@MyAkaId int
declare @allowread bit, @allowwrite bit



BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
begin tran
select @AreaId=isnull(max(Areaid),0)+1 from EchoAreas
Select @AreaGroup=DefaultGroup,@MyAkaId=UseAka from Links where LinkId=@LinkId
insert into EchoAreas values(@AreaId, @AreaName,null,@AreaGroup,@MyAkaId,@LinkId,Getdate(),0)
commit tran
select @allowread=allowread,@allowwrite=allowwrite from Grouppermissions where LinkId=@LinkId and AreaGroup=@AreaGroup
insert into arealinks values(@AreaId,@LinkId,@Allowread,@Allowwrite,0,0)
insert into arealinks select @AreaId,LinkId,AllowRead,Allowwrite,0,0 from NewAreaLinks where AreaGroup=@AreaGroup and LinkId<>@LinkId

return @AreaId
END

GO
/****** Object:  StoredProcedure [dbo].[sp_DeleteLink]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_DeleteLink] 
	@zone int, @net int, @node int, @point int=0
AS
declare @linkid int
BEGIN
select @linkid=isnull(LinkID, 0) from Links where Zone=@zone and Net=@net and Node=@node and Point=@point
If @linkid=0 return
delete from AreaLinks where LinkID=@linkid
delete from GroupPermissions where LinkID=@linkid
delete from NewAreaLinks where LinkID=@linkid
delete from Links where LinkID=@linkid

END


GO
/****** Object:  StoredProcedure [dbo].[sp_DirectNetmail]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_DirectNetmail]
	
AS
BEGIN
	
	SET NOCOUNT ON;
	begin tran
    insert into NetmailOutbound select LinkId,MessageId from Links,Netmail where Links.Zone=Netmail.ToZone and Links.Net=Netmail.ToNet and Links.Node=Netmail.ToNode and Links.Point=0 and Links.LinkType>=3 and Netmail.Direct<>0 and Netmail.Sent=0 and Netmail.Locked=0
	update Netmail set Sent=1 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID

	commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_EchoStat]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE PROCEDURE [dbo].[sp_EchoStat] @AreaID int
AS
declare @cnt int
BEGIN
SET NOCOUNT ON;

	select @cnt=Messages from WeeklyEchoStat where AreaId=@AreaID
	if isnull(@cnt,0)=0
		insert into WeeklyEchoStat values(@AreaID,1)
	else 
		update WeeklyEchoStat set Messages=@cnt+1 where Areaid=@AreaID

END


GO
/****** Object:  StoredProcedure [dbo].[sp_FileSent]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_FileSent] @LinkID int, @FileName nvarchar(260)
	
AS
declare @isDelete bit,@cnt int
BEGIN
set nocount on
select @isDelete=KillFileAfterSent from FileOutbound where LinkID=@LinkID and FileName=@FileName
delete from FileOutbound where LinkID=@LinkID and FileName=@FileName
if @isDelete=0 return 0
select @cnt=count(*) from FileOutbound where FileName=@FileName
if @cnt=0 return 1
return 0
	
END

GO
/****** Object:  StoredProcedure [dbo].[sp_GetLinkIdForNetmailRouting]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_GetLinkIdForNetmailRouting] @Zone smallint,@Net smallint,@Node smallint,@Point smallint,@SoftwareCode tinyint
AS
DECLARE @LinkID int	
BEGIN
	SET NOCOUNT ON;

exec sp_LogSession @Zone,@Net,@Node,@Point,@SoftwareCode
if @Point<>0 return 0

select @LinkID=LinkID from Links where Zone=@Zone and Net=@Net and Node=@Node and Point=0 and NetmailDirect<>0 and PassiveLink=0 and LinkType<>2
select @LinkID=isnull(@LinkID,0)
if @LinkID=0
begin
select @LinkID=Links.LinkID from LinksAKAs,Links where LinksAKAs.LinkID=Links.LinkID and LinksAKAs.Zone=@Zone and LinksAKAs.Net=@Net and LinksAKAs.Node=@Node and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink=0 and Links.LinkType<>2
select @LinkID=isnull(@LinkID,0)
end

return @LinkID
END

GO
/****** Object:  StoredProcedure [dbo].[sp_GetMsgIdTime]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_GetMsgIdTime]
AS
declare @retval int,@retval1 int
SET NOCOUNT ON;

begin tran

select @retval=datediff(s,'1.1.1970',getdate())
select @retval1=Value from IntTable where ValueName='MsgIdLastTime'
if (@retval>=@retval1)
	select @retval=@retval+1
else
	select @retval=@retval1+1

update IntTable set Value=@retval where ValueName='MsgIdLastTime'

commit tran

return @retval

GO
/****** Object:  StoredProcedure [dbo].[sp_GetPktNumber]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE PROCEDURE [dbo].[sp_GetPktNumber]
AS
declare @retval int,@retval1 int
SET NOCOUNT ON;
begin tran

select @retval=datediff(s,'1.1.1970',getdate())
select @retval1=Value from IntTable where ValueName='PktLastNum'
if (@retval>=@retval1)
	select @retval=@retval+1
else
	select @retval=@retval1+1

update IntTable set Value=@retval where ValueName='PktLastNum'

commit tran

return @retval

GO
/****** Object:  StoredProcedure [dbo].[sp_LogSession]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE procedure [dbo].[sp_LogSession] @Zone smallint, @Net smallint, @Node smallint, @Point smallint, @Sessiontype int
as
SET NOCOUNT ON;


update Links set LastSessionType=@SessionType, LastSessionTime=Getdate(), TotalSessions=TotalSessions+1, ThisDaySessions=ThisDaySessions+1 where Zone=@Zone and Net=@Net and Node=@Node and Point=@Point
if @Point<>0 return
update Links set LastSessionType=@SessionType, LastSessionTime=Getdate(), TotalSessions=TotalSessions+1, ThisDaySessions=ThisDaySessions+1 from Links,LinksAKAs where Links.LinkID=LinksAKAs.LinkID and LinksAKAs.Zone=@Zone and LinksAKAs.Net=@Net and LinksAKAs.Node=@Node 
return

GO
/****** Object:  StoredProcedure [dbo].[sp_MakeNetmailRouting]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_MakeNetmailRouting]
AS
BEGIN
declare @count int,@count1 int
SET NOCOUNT ON;

DELETE FROM Routing where Static=0
UPDATE NetmailRouTable SET processed=0
UPDATE NetmailTruTable SET processed=0
UPDATE Nodelist SET processed=0

/* отбрасываем роутинг на нас или через нас*/
UPDATE NetmailRouTable SET processed=1 from NetmailRouTable,MyAka where NetmailRouTable.RouteDestZone=MyAka.Zone and NetmailRouTable.RouteDestNet=MyAka.Net and NetmailRouTable.RouteDestNode=MyAka.Node and MyAka.Point=0
UPDATE NetmailRouTable SET processed=1 from NetmailRouTable,MyAka where NetmailRouTable.RouteThruZone=MyAka.Zone and NetmailRouTable.RouteThruNet=MyAka.Net and NetmailRouTable.RouteThruNode=MyAka.Node and MyAka.Point=0

UPDATE NetmailTruTable SET processed=1 from NetmailTruTable,MyAka where NetmailTruTable.ToZone=MyAka.Zone and NetmailTruTable.ToNet=MyAka.Net and NetmailTruTable.ToNode=MyAka.Node and MyAka.Point=0
UPDATE NetmailTruTable SET processed=1 from NetmailTruTable,MyAka where NetmailTruTable.TrustedAddrZone=MyAka.Zone and NetmailTruTable.TrustedAddrNet=MyAka.Net and NetmailTruTable.TrustedAddrNode=MyAka.Node and MyAka.Point=0

UPDATE Nodelist set processed=1 from Nodelist,MyAka where Nodelist.Zone=MyAka.Zone and Nodelist.Net=MyAka.Net and Nodelist.Node=MyAka.Node and MyAka.Point=0

/* отбрасываем роутинг на линков, на которых стоит флаг директ*/

UPDATE NetmailRouTable SET processed=1 from NetmailRouTable,Links where NetmailRouTable.RouteDestZone=Links.Zone and NetmailRouTable.RouteDestNet=Links.Net and NetmailRouTable.RouteDestNode=Links.Node and Links.Point=0 and Links.PassiveLink=0 and Links.NetmailDirect<>0
UPDATE NetmailTruTable SET processed=1 from NetmailTruTable,Links where NetmailTruTable.ToZone=Links.Zone and NetmailTruTable.ToNet=Links.Net and NetmailTruTable.ToNode=Links.Node and Links.Point=0 and Links.PassiveLink=0 and Links.NetmailDirect<>0

UPDATE Nodelist set processed=1 from Nodelist,Links where Nodelist.Zone=Links.Zone and Nodelist.Net=Links.Net and Nodelist.Node=Links.Node and Links.Point=0 and Links.PassiveLink=0 and Links.NetmailDirect<>0

/* прописываем роутинг через линков */
insert into Routing select Linkid,RouteDestZone,RouteDestNet,RouteDestNode,0 from links,NetmailRouTable where Links.Zone=NetmailRouTable.RouteThruZone and Links.Net=NetmailRouTable.RouteThruNet and Links.Node=NetmailRouTable.RouteThruNode and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink = 0 and Links.LinkType<>2 and NetmailRouTable.processed=0
update NetmailRouTable set processed=1 from NetmailRouTable,Routing where NetmailRouTable.RouteDestZone=Routing.Zone and NetmailRouTable.RouteDestNet=Routing.Net and NetmailRouTable.RouteDestNode=Routing.Node

UPDATE Nodelist set processed=1 from Nodelist,Routing where Nodelist.Zone=Routing.Zone and Nodelist.Net=Routing.Net and Nodelist.Node=Routing.Node 
insert into Routing select Linkid,Nodelist.Zone,Nodelist.Net,Nodelist.Node,0 from links,Nodelist where Links.Zone=Nodelist.Zone and Links.Net=Nodelist.Net and Links.Node=Nodelist.HubNode and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink = 0 and Links.LinkType<>2 and processed=0
UPDATE Nodelist set processed=1 from Nodelist,Routing where Nodelist.Zone=Routing.Zone and Nodelist.Net=Routing.Net and Nodelist.Node=Routing.Node 

update NetmailTruTable set processed=1 from NetmailTruTable,Routing where NetmailTruTable.ToZone=Routing.Zone and NetmailTruTable.ToNet=Routing.Net and NetmailTruTable.ToNode=Routing.Node
insert into Routing select Linkid,ToZone,ToNet,ToNode,0 from links,NetmailTruTable where Links.Zone=NetmailTruTable.TrustedAddrZone and Links.Net=NetmailTruTable.TrustedAddrNet and Links.Node=NetmailTruTable.TrustedAddrNode and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink = 0 and Links.LinkType<>2 and NetmailTruTable.processed=0
update NetmailTruTable set processed=1 from NetmailTruTable,Routing where NetmailTruTable.ToZone=Routing.Zone and NetmailTruTable.ToNet=Routing.Net and NetmailTruTable.ToNode=Routing.Node
update NetmailRouTable set processed=1 from NetmailRouTable,Routing where NetmailRouTable.RouteDestZone=Routing.Zone and NetmailRouTable.RouteDestNet=Routing.Net and NetmailRouTable.RouteDestNode=Routing.Node

/* прописываем роутинг через линков линков */
cycle1:
select @count=count(*) from NetmailRouTable where processed=0
insert into Routing select Linkid,RouteDestZone,RouteDestNet,RouteDestNode,0 from Routing,NetmailRouTable where Routing.Zone=NetmailRouTable.RouteThruZone and Routing.Net=NetmailRouTable.RouteThruNet and Routing.Node=NetmailRouTable.RouteThruNode and NetmailRouTable.processed=0
update NetmailRouTable set processed=1 from NetmailRouTable,Routing where NetmailRouTable.RouteDestZone=Routing.Zone and NetmailRouTable.RouteDestNet=Routing.Net and NetmailRouTable.RouteDestNode=Routing.Node
select @count1=count(*) from NetmailRouTable where processed=0
if (@count<>@count1) goto cycle1

update NetmailTruTable set processed=1 from NetmailTruTable,Routing where NetmailTruTable.ToZone=Routing.Zone and NetmailTruTable.ToNet=Routing.Net and NetmailTruTable.ToNode=Routing.Node

cycle2:
select @count=count(*) from NetmailTruTable where processed=0
insert into Routing select Linkid,ToZone,ToNet,ToNode,0 from Routing,NetmailTruTable where Routing.Zone=NetmailTruTable.TrustedAddrZone and Routing.Net=NetmailTruTable.TrustedAddrNet and Routing.Node=NetmailTruTable.TrustedAddrNode and NetmailTruTable.processed=0
update NetmailTruTable set processed=1 from NetmailTruTable,Routing where NetmailTruTable.ToZone=Routing.Zone and NetmailTruTable.ToNet=Routing.Net and NetmailTruTable.ToNode=Routing.Node
select @count1=count(*) from NetmailTruTable where processed=0
if (@count<>@count1) goto cycle2

UPDATE Nodelist set processed=1 from Nodelist,Routing where Nodelist.Zone=Routing.Zone and Nodelist.Net=Routing.Net and Nodelist.Node=Routing.Node 
insert into Routing select Linkid,Nodelist.Zone,Nodelist.Net,Nodelist.Node,0 from links,Nodelist where Links.Zone=Nodelist.Zone and Links.Net=Nodelist.Net and Links.Node=Nodelist.HubNode and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink = 0 and Links.LinkType<>2 and processed=0
UPDATE Nodelist set processed=1 from Nodelist,Routing where Nodelist.Zone=Routing.Zone and Nodelist.Net=Routing.Net and Nodelist.Node=Routing.Node 
insert into Routing select Linkid,Nodelist.Zone,Nodelist.Net,Nodelist.Node,0 from Routing,Nodelist where Routing.Zone=Nodelist.Zone and Routing.Net=Nodelist.Net and Routing.Node=Nodelist.HubNode and processed=0
UPDATE Nodelist set processed=1 from Nodelist,Routing where Nodelist.Zone=Routing.Zone and Nodelist.Net=Routing.Net and Nodelist.Node=Routing.Node 


insert into Routing select Linkid,RouteDestZone,RouteDestNet,RouteDestNode,0 from Routing,NetmailRouTable where Routing.Zone=NetmailRouTable.RouteThruZone and Routing.Net=NetmailRouTable.RouteThruNet and Routing.Node=-1 and NetmailRouTable.processed=0
update NetmailRouTable set processed=1 from NetmailRouTable,Routing where NetmailRouTable.RouteDestZone=Routing.Zone and NetmailRouTable.RouteDestNet=Routing.Net and NetmailRouTable.RouteDestNode=Routing.Node


/*убираем лишние записи, подпадающие под шаблоны*/
delete R1 from Routing as R1,Routing as R2 where R1.Node<>-1 and R2.Node=-1 and R1.Zone=R2.Zone and R1.Net=R2.Net and R1.LinkId=R2.LinkId and R1.Static=0
delete R1 from Routing as R1,Routing as R2 where R2.Node=-1 and R1.Zone=R2.Zone and R1.Net<>-1 and R2.Net=-1 and R1.LinkId=R2.LinkId and R1.Static=0

/*рероутинг*/
begin tran
update Netmail set Sent=0 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID and Netmail.Locked=0
delete from NetmailOutbound where MessageID IN (select NetmailOutbound.MessageId from NetmailOutbound,Netmail where NetmailOutbound.MessageID=Netmail.MessageID and Netmail.Locked=0)
commit tran

END

GO
/****** Object:  StoredProcedure [dbo].[sp_NetmailMessageSent]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_NetmailMessageSent] @MessageID int
as
BEGIN
SET NOCOUNT ON;
declare @ks bit, @visible bit
select @ks=KillSent,@visible=Visible from Netmail where MessageID=@MessageID	
begin tran
delete from NetmailOutbound where MessageID=@MessageID
if @ks<>0 or @visible=0
delete from Netmail where MessageID=@MessageID
else
update Netmail set sent=1,Locked=0 where MessageID=@MessageID
commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_PassiveUnacceptedAreaLink]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_PassiveUnacceptedAreaLink] 
	
	@MessageId int, @LinkId int
AS
declare @AreaId int
BEGIN
	SET NOCOUNT ON;
	select @AreaID=AreaId from EchoMessages where MessageID=@MessageId;
	update AreaLinks set Passive=1 where AreaId=@AreaId and LinkId=@LinkId
	delete from Outbound from EchoMessages where ToLink=@LinkId and EchoMessages.AreaId=@AreaId and OutBound.MessageId=EchoMessages.MessageID
END

GO
/****** Object:  StoredProcedure [dbo].[sp_PostEchomailMessage]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_PostEchomailMessage] (@AreaId int, @FromName nvarchar(36), @ToName nvarchar(36), @FromZone smallint, @FromNet smallint,@FromNode smallint,@FromPoint smallint,
 @Subject nvarchar(74), @MsgId nvarchar(127),@OriginalSize int, @MsgText varbinary(max) )
AS
declare @MessageId int;

BEGIN
	SET NOCOUNT ON;
	update EchoAreas set LastMsgDate=getdate() where AreaId=@AreaId

	begin tran
	select @MessageId=isnull(max(MessageID),0)+1 from EchoMessages
	insert into EchoMessages values(@MessageId, @AreaId, null, @FromName, @ToName,@FromZone, @FromNet, @FromNode, @FromPoint, getdate(), getdate(),@Subject, @MsgId,null,0,0,@OriginalSize, null,null, @MsgText)
	insert into DupeBase values(@AreaId,@MsgId,GetDate())
	commit tran
	exec sp_EchoStat @AreaId
END

GO
/****** Object:  StoredProcedure [dbo].[sp_PostNetmailMsg]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_PostNetmailMsg]
@FromZone smallint, @FromNet smallint, @FromNode smallint, @FromPoint smallint,
@ToZone smallint, @ToNet smallint, @ToNode smallint, @ToPoint smallint,
@FromName nvarchar(36),@ToName nvarchar(36),
@Subject nvarchar(74),
@MsgId nvarchar(127), @ReplyTo nvarchar(127),
@MsgText ntext, 
@killsent bit, @direct bit,
@visible bit
AS
declare @MessageId int
BEGIN
	SET NOCOUNT ON;
begin tran
select @MessageId=isnull(max(messageid),0)+1 from Netmail
insert into Netmail values(@MessageId, @FromZone, @FromNet, @FromNode, @FromPoint,
@ToZone, @ToNet, @ToNode, @ToPoint, GETDATE(), getdate(),@FromName, @ToName,
@Subject, @MsgId, @ReplyTo, @MsgText, 0, 0, @visible, 0, @killsent, 1, 0, 0, 0,
0, @direct, 0,0)


commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_RescanEchoArea]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_RescanEchoArea] @LinkId int, @AreaId int, @NumOfDays int, @Result int output 
	
AS
declare @MessageID int,@tmp int

BEGIN
SET NOCOUNT ON;

	select @Result=0;
	

	declare Resc cursor local read_only for select MessageId from EchoMessages where AreaId=@AreaId and DATEDIFF(D,ReceiveTime,GetDate())<@NumOfDays
	open Resc
	FETCH NEXT FROM Resc into @MessageId
	WHILE @@FETCH_STATUS = 0
	BEGIN 

		select @tmp=count(*) from outbound where ToLink=@LinkId and MessageId=@MessageID

		if @tmp=0
		begin 
			select @Result=@Result+1
			insert into Outbound values (@LinkId,@MessageID,0)
		end 
	FETCH NEXT FROM Resc into @MessageId
	end 
	close Resc
	deallocate Resc
END
GO
/****** Object:  StoredProcedure [dbo].[sp_RouteNetmail]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_RouteNetmail]
AS
BEGIN
begin tran
update Netmail set Sent=1 from Netmail,MyAka where Netmail.ToZone=MyAka.Zone and Netmail.ToNet=Myaka.Net and Netmail.ToNode=MyAka.Node and Netmail.ToPoint=MyAka.Point
insert into NetmailOutbound select LinkId,MessageId from Links,Netmail where Links.Zone=Netmail.ToZone and Links.Net=Netmail.ToNet and Links.Node=Netmail.ToNode and Links.Point=0 and Links.NetmailDirect<>0 and Links.PassiveLink=0 and Netmail.Direct=0 and Netmail.Sent=0
update Netmail set Sent=1 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID

insert into NetmailOutbound select LinkId,MessageId from Routing,Netmail where Routing.Zone=Netmail.ToZone and Routing.Net=Netmail.ToNet and Routing.Node=Netmail.ToNode and Netmail.Direct=0 and Netmail.Sent=0
update Netmail set Sent=1 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID

insert into NetmailOutbound select LinkId,MessageId from Routing,Netmail where Routing.Zone=Netmail.ToZone and Routing.Net=Netmail.ToNet and Routing.Node=-1 and Netmail.Direct=0 and Netmail.Sent=0
update Netmail set Sent=1 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID

insert into NetmailOutbound select LinkId,MessageId from Routing,Netmail where Routing.Zone=Netmail.ToZone and Routing.Net=-1 and Routing.Node=-1 and Netmail.Direct=0 and Netmail.Sent=0
update Netmail set Sent=1 from Netmail,NetmailOutbound where Netmail.MessageID=NetmailOutbound.MessageID

commit tran
END

GO
/****** Object:  StoredProcedure [dbo].[sp_SetCallSuccessStatus]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[sp_SetCallSuccessStatus] @LinkID int,@CallStatus tinyint
AS
declare @pause int
BEGIN
SET NOCOUNT ON;

if @CallStatus=0
update Links set LastCallStatus=0,FailedCallsCount=0,HoldUntil=0, isBusy=0, isCalling=0 where LinkID=@LinkID
else
begin
select @pause=POWER(2,FailedCallsCount) from Links where LinkID=@LinkID
if @Pause>600 select @Pause=600
update Links set LastCallStatus=@CallStatus,FailedCallsCount+=1,HoldUntil=DATEADD(MINUTE,@pause,Getdate()),isBusy=0,isCalling=0 where LinkID=@LinkID
end

update OutBound set Status=0 where ToLink=@LinkID
update Netmail set Locked=0 where Locked=@LinkID

END

GO
/****** Object:  StoredProcedure [dbo].[sp_startup]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE PROCEDURE [dbo].[sp_startup]
as
BEGIN
SET NOCOUNT ON;
update Links set isBusy=0,isCalling=0
update OutBound set Status=0
update netmail set Locked=0

END

GO
/****** Object:  StoredProcedure [dbo].[SubscribeEchoArea]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[SubscribeEchoArea] @LinkId int, @AreaId int, @Result int output 
	
AS
declare @AreaGroup int, @AllowRead bit, @AllowWrite bit, @tmp int

BEGIN
	
	SET NOCOUNT ON;
	select @tmp=count(AreaName) from EchoAreas where AreaID=@AreaId
	if @tmp=0
	begin
		select @Result=3
		return
	end

	select @tmp=count(AllowRead) from AreaLinks where AreaId=@AreaId and LinkId=@LinkID
	if @tmp<>0 begin
		select @Result=0
		return
	end
	
	select @AreaGroup=AreaGroup from EchoAreas where AreaId=@AreaId
	

	select @tmp=count(AllowRead) from GroupPermissions where LinkId=@LinkID and AreaGroup=@AreaGroup; 
	if @tmp=0
	begin
		select @Result=2
		return
	end

	select @AllowRead=isnull(AllowRead,0), @AllowWrite=isnull(AllowWrite,0) from GroupPermissions where LinkId=@LinkID and AreaGroup=@AreaGroup;
	if @AllowRead=0
	begin
		select @Result=2
		return
	end
	insert into AreaLinks values(@AreaID,@LinkID,@AllowRead,@AllowWrite,0,0)
	select @Result=1
	return
    
END

GO
/****** Object:  StoredProcedure [dbo].[UnsubscribeEchoArea]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
/*
0 - не были подписаны
1 - все ок
2 - отписка невозможна, mandatory
*/
CREATE PROCEDURE [dbo].[UnsubscribeEchoArea] @LinkId int, @AreaId int, @Result int output 
	
AS
declare @Mandatory bit, @tmp int

BEGIN
	


	SET NOCOUNT ON;
	
	select @tmp=count(AreaName) from EchoAreas where AreaID=@AreaId
	if @tmp=0
	begin
		select @Result=3
		return
	end
		
	select @tmp=count(AllowRead) from AreaLinks where AreaId=@AreaId and LinkId=@LinkID
	if @tmp=0 begin
		select @Result=0
		return
	end

	
	select @Mandatory=Mandatory from AreaLinks where AreaID=@AreaID and LinkId=@LinkID
	if @Mandatory<>0
	begin
		select @result=2
		return
	end

	delete from AreaLinks where AreaID=@AreaID and LinkId=@LinkID
	select @Result=1
	return
    
END
GO
/****** Object:  StoredProcedure [dbo].[UpdateLinkSettings]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[UpdateLinkSettings] @LinkId int,@SessionPwd nvarchar(64), @AreafixPwd nvarchar(64), @MaxPktSize int, @MaxArchiveSize int, @Ip nvarchar(256),@NetmailDirect bit, @AutoSubscribe bit,@PktPwd varchar(9),@UsePktPwd bit
	
AS
declare @Dialout bit,@cnt int,@allowread bit, @allowwrite bit
BEGIN
	SET NOCOUNT ON;
	
	if len(isnull(@ip,''))=0 select @Dialout=0
	else select @Dialout=1
		
	update Links set SessionPassword=@SessionPwd, AreaFixPassword=@AreafixPwd,PktPassword=@PktPwd,UsePktPassword=@UsePktPwd,MaxPktSize=@MaxPktSize,MaxArchiveSize=@MaxArchiveSize,Ip=@Ip,DialOut=@Dialout,NetmailDirect=@NetmailDirect where LinkID=@LinkId
    if @AutoSubscribe=0 delete from NewAreaLinks where LinkId=@LinkId and AreaGroup=0
	
	else
	begin
		select @cnt=count(LinkID) from NewAreaLinks where LinkId=@LinkId and AreaGroup=0
		if @cnt=0 
		begin
			select @allowread=allowread, @allowwrite=allowwrite from GroupPermissions where LinkId=@LinkId and AreaGroup=0
			insert into NewAreaLinks values(0,@LinkId,@allowread,@allowwrite)
		end

	end
END

GO
/****** Object:  StoredProcedure [dbo].[ViewEchoMessage]    Script Date: 28.08.2014 20:17:46 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[ViewEchoMessage] @MessageID int,@AreaID int=0 output, @AreaName nvarchar(255) = '' output, @NavFirstID int=0 output, @NavLastID int=0 output, @NavPrevID int=0 output, @NavNextID int=0 output, @NavReplyID int=0 output
	
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;
	declare @ReplyTo nvarchar(127)

	select @AreaID=isnull(AreaId,0), @ReplyTo=isnull(ReplyTo,'') from EchoMessages where MessageID=@MessageID
	if @AreaId=0 return 0
	
	select @AreaName=isnull(AreaName,'') from EchoAreas where AreaID=@AreaID
	if @ReplyTo=''
		select @NavReplyID=0
	else
		select @NavReplyID=isnull(MessageID,0) from EchoMessages where AreaId=@AreaID and MsgId=@ReplyTo

	select @NavFirstID=min(MessageID) from EchoMessages where AreaId=@AreaID
	if @NavFirstID=@MessageID select @NavFirstID=0

	select @NavLastID=max(MessageID) from EchoMessages where AreaId=@AreaID
	if @NavLastID=@MessageID select @NavLastID=0

	select @NavPrevID=isnull(max(MessageID),0) from EchoMessages where AreaId=@AreaID and MessageId<@MessageID;
	select @NavNextID=isnull(min(MessageID),0) from EchoMessages where AreaId=@AreaID and MessageId>@MessageID;

    

END

GO
